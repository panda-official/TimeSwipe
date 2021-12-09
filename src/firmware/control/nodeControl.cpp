/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "control/DataVis.h"
#include "control/nodeControl.h"
#include "sam/SamService.h"


nodeControl::nodeControl()
{
    m_pMesChans.reserve(4);

#ifdef CALIBRATION_STATION
    m_bCalEnabled=false;
#else
    m_bCalEnabled=true;
#endif

}

void nodeControl::SetEEPROMiface(const std::shared_ptr<ISerial> &pBus, const std::shared_ptr<CFIFO> &pMemBuf)
{
    m_EEPROMstorage.set_buf(pMemBuf);
    m_pEEPROMbus=pBus;

    if (m_EEPROMstorage.verify() != hat::Manager::Op_result::ok) {
      m_EEPROMstorage.reset();
      hat::atom::Vendor_info vinf{CSamService::GetSerial(), 0, 2, "Panda", "Timeswipe"};
      m_EEPROMstorage.set(vinf); //storage is ready
    }

    //fill blank atoms with the stubs:
    for (int i = m_EEPROMstorage.atom_count(); i < 3; ++i) {
      hat::atom::Stub stub{i};
      m_EEPROMstorage.set(stub);
    }

    hat::Calibration_map map;
    m_CalStatus = m_EEPROMstorage.get(map);
    if (m_CalStatus == hat::Manager::Op_result::ok) {
      std::string err;
      ApplyCalibrationData(map, err);
    }
}

void nodeControl::ApplyCalibrationData(const hat::Calibration_map& map, std::string& err)
{
  if (!m_bCalEnabled) {
    err = "calibration settings are disabled";
    return;
  }

  if (m_pVoltageDAC) {
    const auto& atom = map.atom(hat::atom::Calibration::Type::v_supply);
    if (atom.entry_count() != 1) { // exactly 1 entry per specification
      err = "invalid v_supply calibration atom";
      return;
    }
    const auto& entry = atom.entry(0);
    m_pVoltageDAC->SetLinearFactors(entry.slope(), entry.offset());
    m_pVoltageDAC->SetVal();
  }

  // Update channels.
  for (auto& el : m_pMesChans) el->update_offsets();
}

bool nodeControl::SetCalibrationData(hat::Calibration_map& map, std::string& err)
{
  m_CalStatus = m_EEPROMstorage.set(map);
  if (m_CalStatus != hat::Manager::Op_result::ok) {
    err = "invalid calibration map";
    return false;
  }

  ApplyCalibrationData(map, err);
  if (!err.empty()) return false;

  if (!m_pEEPROMbus->send(*m_EEPROMstorage.buf())) {
    err = "failed to write EEPROM";
    return false;
  }

  return true;
}

bool nodeControl::GetCalibrationData(hat::Calibration_map& data, std::string& strError)
{
  using Op_result = hat::Manager::Op_result;
  if (const auto r = m_EEPROMstorage.get(data);
    r == Op_result::ok || r == Op_result::atom_not_found)
    return true;

  strError = "EEPROM image is corrupted";
  return false;
}

bool nodeControl::_procCAtom(rapidjson::Value& jObj, rapidjson::Document& jResp, const CCmdCallDescr::ctype ct, std::string &strError)
{
    hat::Calibration_map map;

    //load existing atom
    auto& nc = nodeControl::Instance();
    if (!nc.GetCalibrationData(map, strError))
      return false;

    const auto catom = jObj["cAtom"].GetUint();
    const auto type = hat::atom::Calibration::to_type(catom, strError);
    if (!strError.empty()) return false;

    const auto cal_entry_count = map.atom(type).entry_count();

    //if call type=set
    if(CCmdCallDescr::ctype::ctSet==ct)
    {
#ifndef CALIBRATION_STATION
        strError="calibration setting is prohibited!";
        return false;
#endif

        auto& data = jObj["data"];
        const auto data_size = data.Size();
        if (data_size > cal_entry_count) {
          strError = "wrong data count";
          return false;
        }

        for (std::size_t i{}; i < data_size; ++i) {
          const auto& el = data[i];
          auto entry = map.atom(type).entry(i);
          if (const auto it = el.FindMember("m"); it != el.MemberEnd())
            entry.set_slope(it->value.GetFloat());
          if (const auto it = el.FindMember("b"); it != el.MemberEnd())
            entry.set_offset(it->value.GetInt());
          map.atom(type).set_entry(i, std::move(entry));
        }

        // Save the calibration map.
        if (!nc.SetCalibrationData(map, strError)) return false;
    }

    // Generate data member of the response.
    auto& alloc = jResp.GetAllocator();
    rapidjson::Value data{rapidjson::kArrayType};
    data.Reserve(cal_entry_count, alloc);
    for (std::size_t i{}; i < cal_entry_count; ++i) {
      const auto& entry = map.atom(type).entry(i);
      const auto m = entry.slope();
      const auto b = entry.offset();
      data.PushBack(std::move(rapidjson::Value{rapidjson::kObjectType}
          .AddMember("m", m, alloc)
          .AddMember("b", b, alloc)), alloc);
    }

    // Fill the response.
    jResp.RemoveAllMembers();
    jResp.AddMember("cAtom", catom, alloc);
    jResp.AddMember("data", std::move(data), alloc);

    return true;
}

void nodeControl::procCAtom(rapidjson::Value& jObj, rapidjson::Document& jResp,
  const CCmdCallDescr::ctype ct)
{
  std::string err;
  if (!_procCAtom(jObj, jResp, ct, err)) {
    using Value = rapidjson::Value;
    auto& alloc = jResp.GetAllocator();
    jResp.AddMember("cAtom", Value{}, alloc);
    set_error(jResp, jResp["cAtom"], err);
  }
}

void nodeControl::Serialize(CStorage &st)
{
    m_OffsetSearch.Serialize(st);
    if(st.IsDefaultSettingsOrder())
    {
        SetGain(1);
        SetBridge(false);
        SetSecondary(0);
    }

    st.ser(m_GainSetting).ser(m_BridgeSetting).ser(m_SecondarySetting);

    if(st.IsImporting())
    {
        SetGain(m_GainSetting);
        SetBridge(m_BridgeSetting);
        SetSecondary(m_SecondarySetting);
    }
}

void nodeControl::Update()
{
    for(auto& el : m_pMesChans) el->update();

    m_PersistStorage.Update();
    m_OffsetSearch.Update();
}
void nodeControl::StartRecord(bool how)
{
    //make a stamp:
    static unsigned long count_mark=0;
    count_mark++;

    //generate an event:
    rapidjson::Value v{std::uint64_t{count_mark}};
    Instance().Fire_on_event("Record", v);
}

int nodeControl::gain_out(int val)
{
    //update channels gain setting:
    float gval=val;
    m_GainSetting=val;
    for(auto& el : m_pMesChans) el->set_amplification_gain(gval);


     //set old IEPE gain:
     if(typeBoard::IEPEBoard==m_BoardType)
     {
         int gset=val-1;
         m_pGain1pin->write(gset>>1);
         m_pGain0pin->write(gset&1);
     }


     //generate an event:
     rapidjson::Value v{val};
     Instance().Fire_on_event("Gain", v);

     return val;
}
bool nodeControl::GetBridge() const noexcept
{
    return m_BridgeSetting;
}
void nodeControl::SetBridge(bool how)
{
     m_BridgeSetting=how;

     if(typeBoard::IEPEBoard!=m_BoardType)
     {
        assert(m_pUBRswitch);
        m_pUBRswitch->write(how);
     }


    //generate an event:
    rapidjson::Value v{how};
    Instance().Fire_on_event("Bridge", v);
}

void nodeControl::SetSecondary(int nMode)
{
    nMode&=1; //fit the value

    m_SecondarySetting=nMode;
}
int nodeControl::GetSecondary() const noexcept
{
    return m_SecondarySetting;
}


void nodeControl::SetMode(int nMode)
{
    m_OpMode=static_cast<MesModes>(nMode);
    if(m_OpMode<MesModes::IEPE) { m_OpMode=MesModes::IEPE; }
    if(m_OpMode>MesModes::Normsignal){ m_OpMode=MesModes::Normsignal; }


    if(typeBoard::IEPEBoard==m_BoardType) //old IEPE board setting
    {
        //SetBridge(m_OpMode);
        assert(m_pUBRswitch);
        m_pUBRswitch->write(MesModes::IEPE==m_OpMode ? true:false);
    }

    //switch all channels to IEPE:
    for(auto& el : m_pMesChans) el->set_iepe(MesModes::IEPE==m_OpMode);

    SetSecondary(m_OpMode);

    //generate an event:

    rapidjson::Value v{nMode};
    Instance().Fire_on_event("Mode", v);
}
int nodeControl::GetMode() const noexcept
{
    return m_OpMode;
}


void nodeControl::SetOffset(int nOffs)
{
    switch(nOffs)
    {
        case 1: //negative
            m_OffsetSearch.Start(4000);
        break;

        case 2: //zero
            m_OffsetSearch.Start();
        break;

        case 3: //positive
            m_OffsetSearch.Start(100);
        break;

        default:
            nOffs=0;
            m_OffsetSearch.StopReset();
        return;
    }

    rapidjson::Value v{nOffs};
    Instance().Fire_on_event("Offset", v);
}
