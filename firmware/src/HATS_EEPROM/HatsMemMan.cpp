/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include <stdint.h>
#include <string.h>
#include "HatsMemMan.h"

bool CHatAtomVendorInfo::load(CFIFO &buf)
{
    //special deserialization:
    int nDSize=buf.in_avail();
    if(nDSize<22)
        return false;
    //const char *pData=buf.data();

    typeSChar ch;
  /*  uint8_t   *pBuf=(uint8_t *)(m_uuid);
    for(int i=0; i<20; i++)
    {
        buf>>ch;
        pBuf[i]=(uint8_t)ch;
    }*/

    uint8_t   *pBuf=(uint8_t *)m_uuid.data();
    for(int i=0; i<16; i++)
    {
        buf>>ch;
        pBuf[i]=(uint8_t)ch;
    }
    typeSChar b0, b1;
    buf>>b0>>b1;
    *((uint8_t*)&m_PID)=b0; *( ((uint8_t*)&m_PID) +1 )=b1;
    buf>>b0>>b1;
    *((uint8_t*)&m_pver)=b0; *( ((uint8_t*)&m_pver) +1 )=b1;

    int vslen, pslen;
    buf>>vslen>>pslen;

    m_vstr.reserve(vslen);
    m_pstr.reserve(pslen);

    for(int i=0; i<vslen; i++)
    {
        buf>>ch;
        m_vstr+=ch;
    }
    for(int i=0; i<pslen; i++)
    {
        buf>>ch;
        m_pstr+=ch;
    }
    return true;

}
bool CHatAtomVendorInfo::store(CFIFO &buf)
{
    //special serialization:
    int vslen=m_vstr.length();
    int pslen=m_pstr.length();
    buf.reserve(22+vslen+pslen); //minimum size
   /* uint8_t   *pBuf=(uint8_t *)(m_uuid);
    for(int i=0; i<20; i++)
    {
        buf<<pBuf[i];
    }*/

    uint8_t   *pBuf=(uint8_t *)m_uuid.data();
    for(int i=0; i<16; i++)
    {
        buf<<pBuf[i];
    }
    buf<<*((uint8_t*)&m_PID)<<*( ((uint8_t*)&m_PID) +1 );
    buf<<*((uint8_t*)&m_pver)<<*( ((uint8_t*)&m_pver) +1 );


    buf<<vslen<<pslen;
    for(int i=0; i<vslen; i++)
    {
        buf<<m_vstr[i];
    }
    for(int i=0; i<pslen; i++)
    {
        buf<<m_pstr[i];
    }
    return true;
}
void CHatAtomVendorInfo::reset()
{
    //memset(m_uuid, 0, 20);
    m_uuid.fill(0);
    m_PID=0;
    m_pver=0;
    m_vstr.erase();
    m_pstr.erase();
}


bool CHatAtomGPIOmap::load(CFIFO &buf)
{
    //special deserialization:
    int nDSize=buf.in_avail();
    if(nDSize<30)
        return false;

    typeSChar ch;
    uint8_t   *pBuf=(uint8_t *)(&m_bank_drive);
    for(int i=0; i<30; i++)
    {
        buf>>ch;
        pBuf[i]=(uint8_t)ch;
    }
    return true;
}
bool CHatAtomGPIOmap::store(CFIFO &buf)
{
    //special serialization:
    buf.reserve(30);

    uint8_t   *pBuf=(uint8_t *)(&m_bank_drive);
    for(int i=0; i<30; i++)
    {
        buf<<pBuf[i];
    }
    return true;
}
void CHatAtomGPIOmap::reset()
{
    memset(&m_bank_drive, 0, 30);
}


namespace  {

#include "eeptypes.h"

struct atom_header {
    uint16_t type;
    uint16_t count;
    uint32_t dlen;
   // char data_begin;
};

#define SIGNATURE 0x69502d52
#define VERSION 1
CHatsMemMan::op_result FindAtomHeader(unsigned int nAtom, const char *pMemBuf, const int MemBufSize, struct atom_header **pHeaderBegin)
{
    struct header_t *pHeader=(struct header_t *)(pMemBuf);
    const char *pMemLimit=(pMemBuf+MemBufSize);

    CHatsMemMan::op_result rv=CHatsMemMan::op_result::OK;

    //check if nAtom fits the boundares:
    if(nAtom>=pHeader->numatoms)
    {
        nAtom=pHeader->numatoms;
        rv=CHatsMemMan::op_result::atom_not_found;
    }


    const char *pAtomPtr=(pMemBuf+sizeof (struct header_t));
    for(int i=0; i<nAtom; i++)
    {
        pAtomPtr+=(sizeof(struct atom_header) + ((struct atom_header *)pAtomPtr)->dlen);
        if(pAtomPtr>pMemLimit)
        {
            //return memory violation
            return CHatsMemMan::op_result::storage_is_corrupted;
        }
    }

    *pHeaderBegin=(struct atom_header *)pAtomPtr;  //always return the pointer to the next atom or at least where it should be...
    return rv;
}

CHatsMemMan::op_result VerifyAtom(struct atom_header *pAtom)
{
    //check the atom CRC:
    const unsigned int dlen=pAtom->dlen-2; //real dlen without CRC
    const char *pData=(const char*)pAtom + sizeof(struct atom_header);

    uint16_t calc_crc=getcrc((char*)pAtom, dlen+sizeof(atom_header) );
    uint16_t *pCRC=(uint16_t*)(pData+dlen);
    if(calc_crc!=*pCRC)
        return CHatsMemMan::op_result::atom_is_corrupted;

    return CHatsMemMan::op_result::OK;
}

CHatsMemMan::op_result VerifyStorage(const char *pMemBuf, const int MemBufSize)
{
    if(MemBufSize<sizeof(struct header_t))
        return CHatsMemMan::op_result::storage_is_corrupted;

    struct header_t *pHeader=(struct header_t *)(pMemBuf);
    const char *pMemLimit=(pMemBuf+MemBufSize);

    if(SIGNATURE!=pHeader->signature || VERSION!=pHeader->ver || pHeader->res!=0 || pHeader->eeplen>MemBufSize)
       return  CHatsMemMan::op_result::storage_is_corrupted;

    //verify all atoms:
    int nAtoms=pHeader->numatoms;
    const char *pAtomPtr=(pMemBuf+sizeof (struct header_t));
    for(int i=0; i<nAtoms; i++)
    {
        CHatsMemMan::op_result res=VerifyAtom( (struct atom_header *)pAtomPtr );
        if(CHatsMemMan::op_result::OK!=res)
            return res;
        pAtomPtr+=(sizeof(struct atom_header) + ((struct atom_header *)pAtomPtr)->dlen);
        if(pAtomPtr>pMemLimit)
        {
            //return memory violation
            return CHatsMemMan::op_result::storage_is_corrupted;
        }
    }
    return CHatsMemMan::op_result::OK;
}

CHatsMemMan::op_result  ResetStorage(const char *pMemBuf, const int MemBufSize)
{
    if(MemBufSize<sizeof(struct header_t))
        return CHatsMemMan::op_result::storage_is_corrupted;

    struct header_t *pHeader=(struct header_t *)(pMemBuf);

    pHeader->signature=SIGNATURE;
    pHeader->ver=VERSION;
    pHeader->res=0;
    pHeader->numatoms=0;
    pHeader->eeplen=sizeof(struct header_t);
    return CHatsMemMan::op_result::OK;
}

}

/*CHatsMemMan::CHatsMemMan(std::shared_ptr<CFIFO> &pFIFObuf)
{
    m_pFIFObuf=pFIFObuf;
}*/

unsigned int CHatsMemMan::GetAtomsCount()
{
  struct header_t *pHeader=(struct header_t *)GetMemBuf();
  return pHeader->numatoms;
}
CHatsMemMan::op_result CHatsMemMan::Verify()
{
    op_result res=VerifyStorage(GetMemBuf(), GetMemBufSize());
    m_StorageState=res;
    return res;
}
void CHatsMemMan::Reset()
{
    SetMemBufSize(sizeof(struct header_t));
    m_StorageState=ResetStorage(GetMemBuf(), GetMemBufSize());
}


CHatsMemMan::op_result CHatsMemMan::ReadAtom(unsigned int nAtom, typeHatsAtom &nAtomType, CFIFO &rbuf)
{
    if(OK!=m_StorageState)
        return m_StorageState;

    struct atom_header *pAtom;
    op_result res=FindAtomHeader(nAtom, GetMemBuf(), GetMemBufSize(), &pAtom);
    if(op_result::OK!=res)
        return res;

    //check the atom CRC:
    const unsigned int dlen=pAtom->dlen-2; //real dlen without CRC
    const char *pData=(const char*)pAtom + sizeof(struct atom_header); //&pAtom->data_begin;
    nAtomType=static_cast<typeHatsAtom>(pAtom->type);

    uint16_t calc_crc=getcrc((char*)pAtom, dlen+sizeof(atom_header) );
    uint16_t *pCRC=(uint16_t*)(pData+dlen);
    if(calc_crc!=*pCRC)
        return atom_is_corrupted;

    //fill the output variables:
    for(int i=0; i<dlen; i++)
    {
        rbuf<<pData[i];
    }
    return op_result::OK;
}
CHatsMemMan::op_result CHatsMemMan::WriteAtom(unsigned int nAtom, typeHatsAtom nAtomType, CFIFO &wbuf)
{
    if(OK!=m_StorageState)
        return m_StorageState;


    struct atom_header *pAtom;

    unsigned int nAtomsCount=GetAtomsCount();
    if(nAtom>nAtomsCount)
        return op_result::atom_not_found;

    bool bAddingNew=(nAtom==nAtomsCount);


    const char *pMemBuf=GetMemBuf();
    op_result res=FindAtomHeader(nAtom, pMemBuf, GetMemBufSize(), &pAtom);
    if(bAddingNew)
    {
           if(op_result::atom_not_found!=res)
               return res;
    }
    else if(op_result::OK!=res)
               return res;


    //what can happaned here...if atom is not found this is ok, we can write a new one
    //the problem can be if whole storage is corrupted...
    //should we check it at the beginning?
    //lets assume the storage is OK: FindAtomHeader can check the header, not each atom...
    unsigned int req_size=wbuf.size();
    int nMemAdjustVal;
    if(bAddingNew)
    {
        nMemAdjustVal=req_size+sizeof(atom_header)+2;
        AdjustMemBuf((const char*)pAtom, nMemAdjustVal); //completely new
    }
    else
    {
        int dlen=pAtom->dlen-2;
        nMemAdjustVal=(int)(req_size - dlen);
         AdjustMemBuf((char*)pAtom+sizeof(struct atom_header), nMemAdjustVal); //keep header
    }


    //will it be the same address after realocation?!
    //if not have to repeat Finding procedure...
    if(GetMemBuf()!=pMemBuf)
    {
        pMemBuf=GetMemBuf();
        FindAtomHeader(nAtom, pMemBuf, GetMemBufSize(), &pAtom);
    }

    //after adjustion, fill the atom struct:
    pAtom->type=static_cast<uint16_t>(nAtomType);
    pAtom->count=nAtom; //also zero-based atom count
    pAtom->dlen=req_size+2;
    char *pData=(char*)pAtom+sizeof(struct atom_header);
    uint16_t *pCRC=(uint16_t*)(pData+req_size);
    for(unsigned int i=0; i<req_size; i++)
    {
        pData[i]=wbuf[i];
    }
    *pCRC=getcrc((char*)pAtom, req_size+sizeof(struct atom_header)); //set CRC stamp, atom is ready

    ((struct header_t *)(pMemBuf))->eeplen+=nMemAdjustVal;
    if(bAddingNew)
    {
        //also setup the header with the new data:
        ((struct header_t *)(pMemBuf))->numatoms=nAtom+1;
    }

    return op_result::OK;
}

//memory operations:
const char *CHatsMemMan::GetMemBuf()
{
    return m_pFIFObuf->data();
}
int  CHatsMemMan::GetMemBufSize()
{
    return m_pFIFObuf->size();
}
void CHatsMemMan::SetMemBufSize(int size)
{
    m_pFIFObuf->resize(size);
}
void CHatsMemMan::AdjustMemBuf(const char *pStart, int nAdjustVal)
{
    if(0==nAdjustVal)
        return;

    int req_ind=pStart-m_pFIFObuf->data();
    int size=GetMemBufSize();
    if(nAdjustVal>0)    //insert
    {
        m_pFIFObuf->insert(req_ind, nAdjustVal, 0);
    }
    else                //erase
    {
        m_pFIFObuf->erase(req_ind, -nAdjustVal);
    }
}

