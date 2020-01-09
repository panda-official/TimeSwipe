/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include <stdint.h>
#include "HatsMemMan.h"

namespace  {

#include "eeptypes.h"

struct atom_header {
    uint16_t type;
    uint16_t count;
    uint32_t dlen;
   // char data_begin;
};

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

}

CHatsMemMan::CHatsMemMan(std::shared_ptr<CFIFO> &pFIFObuf)
{
    m_pFIFObuf=pFIFObuf;
}

unsigned int CHatsMemMan::GetAtomsCount()
{
  struct header_t *pHeader=(struct header_t *)GetMemBuf();
  return pHeader->numatoms;
}

CHatsMemMan::op_result CHatsMemMan::ReadAtom(unsigned int nAtom, typeHatsAtom &nAtomType, CFIFO &rbuf)
{
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
    size=GetMemBufSize();
}

