/**
 * @file Conf.cpp
 * @author Thomas Saquet, Florent Poinsaut
 * @date 
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Alert - Engine is a part of the Alert software
 * Copyright (C) 2013-2017
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 * 
 */

#include "Conf.h"

using namespace std;

Conf conf;

Conf::Conf()
{
    setDBPort(0);
}

Conf::Conf(const Conf& orig)
{
    setPath(orig.getPath());
    setDBHost(orig.getDBHost());
    setDBPort(orig.getDBPort());
    setDBName(orig.getDBName());
    setDBUser(orig.getDBUser());
    setDBPassword(orig.getDBPassword());
    setSessConnectParams(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
    setCriticity(orig.getCriticity());
    setAPIHost(orig.getAPIHost());
    setAPIPort(orig.getAPIPort());
    setId(orig.getId());
}

Conf::~Conf()
{
}

bool Conf::readProgramOptions(int argc, char **argv)
{
    bool res = false;
    
    // Declare the supported options.
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
            ("help", "That is where you are, it displays help and quits.")
            ("logfile", boost::program_options::value<string>(), "logfile path")
            ("logcriticity", boost::program_options::value<unsigned short>(), "log criticity level : debug = 1 / info = 2 / warning = 3 / secure = 4 / error = 5/ fatal = 6")
            ("conffile", boost::program_options::value<string>(), "conffile path")
            ;

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << "\n";
    }
    else
    {
#ifdef NDEBUG
        if (vm.count("logfile"))
        {
            logger.setFile(vm["logfile"].as<string>());
        }
        else
        {
            logger.setFile("/var/log/echoes-alert/engine.log");
        }
#endif

        if (vm.count("logcriticity"))
        {
            logger.setType(vm["logcriticity"].as<unsigned short>());
        }

        if (vm.count("conffile"))
        {
            setPath(vm["conffile"].as<string>());
            log("debug") << "[Conf] conf file = " << m_path;
        }

        res = true;
    }

    return res;
}

bool Conf::readConfFile()
{
    bool res = false;

    // Create an empty property tree object
    using boost::property_tree::ptree;
    ptree pt;
    
    // Load the INI file into the property tree. If reading fails
    // (cannot open file, parse error), an exception is thrown.
    try
    {
        boost::property_tree::read_ini(m_path, pt);
        setId(pt.get<long long>("engine.id"));
        m_alerter = pt.get<bool>("engine.alerter");
        m_cleaner = pt.get<bool>("engine.cleaner");
        m_calculator = pt.get<bool>("engine.calculator");
        sleepThreadCheckAlert = pt.get<int>("engine.sleep-alert-reading");
        sleepThreadRemoveOldValues = pt.get<int>("engine.sleep-remove-old-values");
        sleepThreadCalculate = pt.get<int>("engine.sleep-calculate");
        
        setDBHost(pt.get<string>("database.host"));
        setDBPort(pt.get<unsigned>("database.port"));
        setDBName(pt.get<string>("database.dbname"));
        setDBUser(pt.get<string>("database.username"));
        setDBPassword(pt.get<string>("database.password"));
        setSessConnectParams(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);

        setAPIHost(pt.get<string>("api.host"));
        setAPIPort(pt.get<unsigned>("api.port"));

        log("info") << "Conf file loaded";

        res = true;
    }
    catch (boost::property_tree::ini_parser_error const& e)
    {
        log("error") << "boost::property_tree: " << e.what();
        log("fatal") << "Can't load config file";
    }

    return res;
}

bool Conf::isInDB(Echoes::Dbo::Session &session)
{
    bool res = false;

    try
    {
        Wt::Dbo::Transaction transaction(session, true);
        Wt::Dbo::ptr<Echoes::Dbo::Engine> engPtr = session.find<Echoes::Dbo::Engine>()
                .where(QUOTE(TRIGRAM_ENGINE ID)" = ?").bind(m_id)
                .limit(1);
        if (engPtr)
        {
            res = true;
        }

        transaction.commit();
    }
    catch (Wt::Dbo::Exception const& e)
    {
        log("error") << "Wt::Dbo: " << e.what();
    }

    return res;
}

bool Conf::isAlerter() const
{
    return this->m_alerter;
}
bool Conf::isCleaner() const
{
    return this->m_cleaner;
}
bool Conf::isCalculator() const
{
    return this->m_calculator;
}

void Conf::setPath(string path)
{
    m_path = path;

    return;
}

string Conf::getPath() const
{
    return m_path;
}

void Conf::setId(long long id)
{
    m_id = id;
    return;
}

long long Conf::getId() const
{
    return m_id;
}

string Conf::getDBHost() const
{
    return m_dbHost;
}

void Conf::setDBPort(unsigned dbPort)
{
    m_dbPort = dbPort;

    return;
}

void Conf::setDBHost(string dbHost)
{
    m_dbHost = dbHost;

    return;
}

unsigned Conf::getDBPort() const
{
    return m_dbPort;
}

void Conf::setDBName(string dbName)
{
    m_dbName = dbName;

    return;
}

string Conf::getDBName() const
{
    return m_dbName;
}

void Conf::setDBUser(string dbUser)
{
    m_dbUser = dbUser;

    return;
}

string Conf::getDBUser() const
{
    return m_dbUser;
}

void Conf::setDBPassword(string dbPassword)
{
    m_dbPassword = dbPassword;

    return;
}

string Conf::getDBPassword() const
{
    return m_dbPassword;
}

void Conf::setSessConnectParams
(
 string dbHost,
 unsigned dbPort,
 string dbName,
 string dbUser,
 string dbPassword
 )
{   
    m_sessConnectParams = "hostaddr=" + dbHost +
                         " port=" + boost::lexical_cast<string>(dbPort) +
                         " dbname=" + dbName +
                         " user=" + dbUser +
                         " password=" + dbPassword;

    return;
}

string Conf::getSessConnectParams() const
{
    return m_sessConnectParams;
}

string Conf::getAPIHost() const
{
    return m_apiHost;
}

void Conf::setAPIPort(unsigned apiPort)
{
    m_apiPort = apiPort;

    return;
}

void Conf::setAPIHost(string apiHost)
{
    m_apiHost = apiHost;

    return;
}

unsigned Conf::getAPIPort() const
{
    return m_apiPort;
}


void Conf::setCriticity(unsigned short criticity)
{
    m_criticity = criticity;
    return;
}

unsigned short Conf::getCriticity() const
{
    return m_criticity;
}
