/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CJSONStream
*/

#pragma once

#include <nlohmann/json.hpp>
#include "frm_stream.h"

/*!
 * \brief A JSON-based formatted stream
 * \details  The class derived from CFrmStream provides a mechanism for retrieving/storing
 * primitive data types (int, float, std::string, e.t.c) from/to the JSON object in the CFrmStream style:
 * by extraction(>>) and insertion (<<) operators that allows easy integration to the communication system
 */
class CJSONStream : public CFrmStream
{
protected:

    /*!
     * \brief A pointer to a JSON object that acts as "stream buffer" here
     */
    nlohmann::json *m_pJSON=nullptr;

public:
    /*!
     * \brief Extraction operator helper override for this class
     * \param pVar void pointer to an extracted variable
     * \param ti variable type
     */
    virtual void get(void *pVar, const std::type_info &ti);

    /*!
     * \brief Insertion operator helper override for this class
     * \param pVar void pointer to an inserted variable
     * \param ti variable type
     */
    virtual void set(const void *pVar, const std::type_info &ti);

    /*!
     * \brief The class constructor
     * \param A pointer to a JSON object that acts as "stream buffer" here
     */
    CJSONStream(nlohmann::json *pJSON) : CFrmStream(nullptr)
    {
        m_pJSON=pJSON;
    }
};

