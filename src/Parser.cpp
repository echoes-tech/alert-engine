#include "Parser.h"
#include <ctime>

Parser::Parser() {
}

Parser::~Parser() {
}

int Parser::unserializeStructuredData(Wt::Dbo::ptr<Syslog> ptrSyslog)
{
    //[prop@5875 ver=1 probe=12]
    //[res2@5875 offset=15 8-4-5-6-2="543" 8-4-5-6-1="54546"][res1@5875 offset=75 8-4-5-6-2="123" 8-4-5-6-1="pojl"]
    
    size_t oBracket=-1; //sucessives positions of the open bracket
    size_t cBracket=-1; //sucessives positions of the closed bracket
    std::vector<size_t> posBrackets; // the list of the brackets positions
    //TODO: we may be use the -1 value of the previous closed bracket to know the value of the open next one
//    std::wstring tempString; //the substring that contains the sd-element passed in argument
    std::string tempString; //the substring that contains the sd-element passed in argument
    bool prop=false; //we know if the properties sd-element is parsed
    Wt::Dbo::ptr<Syslog> ptrSyslogTmp;
    
    //we search and save the different positions of the brackets in the rawStructuredData
    do
    {
        //SQL session
        {
            Wt::Dbo::Transaction transaction(ToolsEngine::session);
            //we fill the local copy of the syslo pointer with the id of the received syslog
            ptrSyslogTmp = ToolsEngine::session.find<Syslog>().where("\"SLO_ID\" = ?").bind(ptrSyslog.id());
            oBracket = ptrSyslogTmp.get()->sd.value().find('[',oBracket+1);
            cBracket = ptrSyslogTmp.get()->sd.value().find(']',cBracket+1);
        }
        
        if ( oBracket != -1 || cBracket != -1 )
        {
                posBrackets.push_back(oBracket) ; //we save the position of the open bracket in the list
                posBrackets.push_back(cBracket) ; //we save the position of the close bracket in the list
        }   
    }while (oBracket != -1); 

    for (int i = 0 ; i < posBrackets.size() ; i +=2 )
    {   
        //SQL session
        {
            Wt::Dbo::Transaction transaction(ToolsEngine::session);
            tempString.assign(ptrSyslogTmp.get()->sd.toUTF8().substr(posBrackets.at(i)+1,posBrackets.at(i+1)-posBrackets.at(i)-1));           
//            tempString.assign(ptrSyslogTmp.get()->sd.value().substr(posBrackets.at(i)+1,posBrackets.at(i+1)-posBrackets.at(i)-1));           
        }
        
        if (posBrackets.at(i) == 0 && prop==false )
        {     
            //we parse the first sd-element containing the properties of the message
            unserializeProperties(tempString, ptrSyslogTmp);
            tempString.clear();
            //we have parsed the properties sd-element
            prop=true;
        }
        else{
            unserializeSDElement(tempString, ptrSyslog);
            tempString.clear();
        }     
    }
}

int Parser::unserializeProperties(std::string& strProperties, Wt::Dbo::ptr<Syslog> ptrSyslog)
{
    //example of string to parse : prop@5875 ver=1 probe=12
    //we parse the first sd-element
    //table to save the equal positions in the first sd-element
    size_t tbEquals[1];
    size_t space;
    tbEquals[0] = strProperties.find('=',0);
    space = strProperties.find(' ',tbEquals[0]+1);
    tbEquals[1] = strProperties.find('=',space);
    Wt::Dbo::ptr<Probe> ptrProbe;
    int idProbeTmp;
    Wt::Dbo::ptr<Syslog> ptrSyslogTmp;
    
    
    //parse idProbe :
    idProbeTmp = ToolsEngine::stringToInt(strProperties.substr(tbEquals[1]+1,strProperties.length()-(tbEquals[1]+1)));
    
    //SQL session
    {
        Wt::Dbo::Transaction transaction(ToolsEngine::session);
        //we fill the local copy of the syslo pointer with the id of the received syslog
        ptrSyslogTmp = ToolsEngine::session.find<Syslog>().where("\"SLO_ID\" = ?").bind(ptrSyslog.id());
        ptrSyslogTmp.modify()->version = ToolsEngine::stringToInt(strProperties.substr(tbEquals[0]+1,space-(tbEquals[0]+1)));      
        
        //we find the probe that have the id of the received syslog and get its pointer
        ptrProbe = ToolsEngine::session.find<Probe>().where("\"PRB_ID\" = ?").bind(idProbeTmp);             
        
        //we modify the probe in the local copy of the syslog pointer with the getted object
        try 
        {                   
            ptrSyslogTmp.modify()->probe = ptrProbe;
        }
        catch (Wt::Dbo::Exception e)
        {
            std::cout << e.what() << "\n";
        }
    } 
    return 0;
}

int Parser::unserializeSDElement(std::string& strSDElement, Wt::Dbo::ptr<Syslog> ptrSyslog)
{
    //string to parse : res2@5875 offset=15 8-4-5-6-2="543" 8-4-5-6-1="54546"
    int offset=0;
    std::vector<std::string> idsPlusValue; // the list of 8-4-5-6-2="543" ids="val"
    std::vector<size_t> spaces; // the list of the spaces in the sub string
    size_t space=-1; //sucessives positions of the space
    std::string tempString; //temp string to save the value parsed before adding to the vector
    
    //we search and save the different positions of the spaces in the strSDElement
    do
    {
        space = strSDElement.find(" ",space+1);

        if ( space != -1 )
        {
                spaces.push_back(space) ; //we save the position of the space in the list
        }   
    }while (space != -1); 
    
    //get the offset
    offset = ToolsEngine::stringToInt(strSDElement.substr(strSDElement.find("=",spaces.at(0))+1,spaces.at(1)-1));
    std::cout << "offset : " << offset << std::endl;
    
    //we add the only element to the vector
    if (spaces.size() == 2)
    {
        idsPlusValue.push_back(strSDElement.substr(spaces.back()+1,strSDElement.size()-spaces.back()));
        std::cout << "idsPlusValue : " << strSDElement.substr(spaces.back()+1,strSDElement.size()-spaces.back()) <<std::endl; 
    }
    //
    else
    {
        for ( int i= 1 ; i < spaces.size()-1 ; i ++)
        {  
            idsPlusValue.push_back(strSDElement.substr(spaces.at(i)+1,spaces.at(i+1)-spaces.at(i)));
            std::cout << "idsPlusValue : " << strSDElement.substr(spaces.at(i)+1,spaces.at(i+1)-spaces.at(i)) <<std::endl;
        }
        idsPlusValue.push_back(strSDElement.substr(spaces.back()+1,strSDElement.size()-spaces.back()));
        std::cout << "idsPlusValue : " << strSDElement.substr(spaces.back()+1,strSDElement.size()-spaces.back()) <<std::endl; 
    }
    //we unserialize each value
    std::cout << "affichage en at : " << idsPlusValue.at(0)<<"\n";
    for ( int i= 0 ; i < idsPlusValue.size() ; i ++)
    {  
        unserializeValue(idsPlusValue.at(i),offset, ptrSyslog);

    }
    return 0;
}

int Parser::unserializeValue(std::string& strValue, int offset, Wt::Dbo::ptr<Syslog> ptrSyslog)
{
    std::cout << "strValue au dbt fonction unserialize value : " << strValue <<"\n";
    //reinit the temp Value
    int idAsset;
    int idPlugin;
    int idSearch;
    int idSource;
    int numSubSearch;
    std::string sValue;
    Wt::WDateTime creaDate;
    
    //string to parse : 8-4-5-6-2="543"  
   
    //table to save the dashs positions between the ids
    size_t tbDashs[3];
    size_t dash=-1;
    
    //table to save the quote positions in the value field
    size_t tbQuotes[1]; 
    size_t quote=-1;    
    
    //we search and save the different positions of the dashs in the strValue
    for(int i = 0 ; i < 4 ; i ++)
    {
        dash = strValue.find("-",dash+1);
        
        if ( dash != -1 )
        {
                tbDashs[i]=dash ; //we save the position of the space in the list
        }
    }
    
    //we search and save the different positions of the quotes in the strValue
    for(int i = 0 ; i < 2 ; i ++)
    {
        quote = strValue.find("\"",quote+1);
        
        if ( quote != -1 )
        {
                tbQuotes[i]=quote ; //we save the position of the space in the list
        }
    }
    
              std::cout << "avt appel idPlugin\n";
      std::cout << "strValue : " << strValue <<"\n";        
      idPlugin = ToolsEngine::stringToInt(strValue.substr(0,tbDashs[0]));
      std::cout << "idplugin : " << idPlugin << "avt appel idAsset\n";
      idAsset = ToolsEngine::stringToInt(strValue.substr(tbDashs[0]+1,tbDashs[1]-(tbDashs[0]+1)));   
      std::cout << "avt appel idSource\n";
      idSource = ToolsEngine::stringToInt(strValue.substr(tbDashs[1]+1,tbDashs[2]-(tbDashs[1]+1))); 
      std::cout << "avt appel idSearch\n";
      idSearch = ToolsEngine::stringToInt(strValue.substr(tbDashs[2]+1,tbDashs[3]-(tbDashs[2]+1)));
      std::cout << "avt appel idnumSubSearch\n";
      numSubSearch = ToolsEngine::stringToInt(strValue.substr(tbDashs[3]+1,tbDashs[4]-(tbDashs[3]+1)));
      std::cout << "avt appel Value\n";
      sValue = Wt::Utils::base64Decode(strValue.substr(tbQuotes[0]+1,tbQuotes[1]-(tbQuotes[0]+1)));
   
   
    //SQL session
    {
        Wt::Dbo::Transaction transaction(ToolsEngine::session);
        informationValueTmp = new InformationValue();
        Wt::Dbo::ptr<Information2> ptrInfTmp = ToolsEngine::session.find<Information2>()
                .where("\"PLG_ID_PLG_ID\" = ?").bind(idPlugin)
                .where("\"SRC_ID\" = ?").bind(idSource)
                .where("\"SEA_ID\" = ?").bind(idSearch)
                .where("\"SUB_SEA_NUM\" = ?").bind(numSubSearch);
        
        informationValueTmp->information = ptrInfTmp;
        informationValueTmp->value = sValue;
        
        //get sent date of the the associated syslog
        creaDate = ptrSyslog.get()->sentDate.addSecs(offset) ;

        informationValueTmp->creationDate = creaDate;
        
        informationValueTmp->syslog = ptrSyslog;
        
        Wt::Dbo::ptr<Asset> ptrAstTmp = ToolsEngine::session.find<Asset>()
                .where("\"AST_ID\" = ?").bind(idAsset);
        
        informationValueTmp->asset = ptrAstTmp;
        
        ToolsEngine::session.add(informationValueTmp);
    }
   
    
// template    
//    //SQL session
//    {
//        Wt::Dbo::Transaction transaction(ToolsEngine::session);
//        //we fill the local copy of the syslo pointer with the id of the received syslog
//        ptrSyslogTmp = ToolsEngine::session.find<Syslog>().where("\"SLO_ID\" = ?").bind(ptrSyslog.id());
//        ptrSyslogTmp.modify()->version = ToolsEngine::wstringToInt(strProperties.substr(tbEquals[0]+1,space-(tbEquals[0]+1)));      
//        
//        //we find the probe that have the id of the received syslog and get its pointer
//        ptrProbe = ToolsEngine::session.find<Probe>().where("\"PRB_ID\" = ?").bind(idProbeTmp);             
//        
//        //we modify the probe in the local copy of the syslog pointer with the getted object
//        try 
//        {                   
//            ptrSyslogTmp.modify()->probe = ptrProbe;
//        }
//        catch (Wt::Dbo::Exception e)
//        {
//            std::cout << e.what() << "\n";
//        }
//    }
    
    
   // std::cout << "Value unserialized : " << tempValue.getIdPlugin() << "-" << tempValue.getIdAsset() << "-" << tempValue.getIdSource() << "-" << tempValue.getIdSearch() << "-" << tempValue.getNumSubSearch() << "=" << "\""<< tempValue.getValue() << "\""<< std::endl; 
    return 0;
}