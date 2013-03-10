/** 
 * 
 * This class parse the datas received in the SYSLOG table of the database and put its in the correct tables
 * A structured data represent a received log : a syslog object and 
 * multiples values.
 * 
 * @author ECHOES Technologies (RHI)
 * @date 2012-05-08
 */

#ifndef PARSER_H
#define	PARSER_H

#include "information/Syslog.h"
#include "probe/Probe.h"
#include <tools/Session.h>
#include <string>
#include <queue>
#include <iostream>
#include <Wt/Dbo/Dbo>
#include <Wt/WDateTime>
#include <Wt/Utils>
#include "ToolsEngine.h"
#include <information/InformationValue.h>
#include <information/Information.h>
#include <asset/Asset.h>
#include <plugin/SearchUnit.h>

class Parser {
public:
    /**
     * class' builder
     */
    Parser();
    virtual ~Parser();
    /**
     * example of a rawStructuredData : 
     * [res2@5875 offset=10 8-4-5-6-2="53" 8-4-5-6-1="987"]
     * [res1@5875 offset=15 8-4-5-6="543" 8-4-5-6="54546"]
     */

    /**
     * method to fill the value's queue with the initial rawStructuredData 
     * @param the pointer to the syslog we want to parse
     * @return error or success
     */
    int unserializeStructuredData(long long id);

    /**
     * method to unserialize the SD Elements included in a rawStructuredData
     * @param strSDElement the string that contains the informations
     * @param the pointer to the syslog we want to parse
     * @return error or success
     */
    int unserializeSDElement(std::string& strSDElement, long long ptrSyslogId);

    /**
     * method to unserialize a value included in a SDElement
     * @param strValue the string that contains the informations
     * @param offset the offet contained on the sd-element to calculate the date
     * @param the pointer to the syslog we want to parse
     * @return error or success
     */
    int unserializeValue(std::string& strValue, int offset, long long ptrSyslogId);

    /**
     * method to unserialize the sd-element concerning the properties
     * @param strProperties the sub string of rawStructuredData that contains the properties
     * @param the pointer to the syslog we want to parse, to fill the properties inside
     * @return error or success
     */
    int unserializeProperties(std::string& strProperties, long long ptrSyslogId);

private:

    //concerning value
    int idAsset;
    int idPlugin;
    int idSearch;
    int idSource;
    int valueNum;
    int lotNumber;
    int lineNumber;
    std::string sValue;


};

#endif	/* PARSER_H */

