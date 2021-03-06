/**
 * @file Server.cpp
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

#include "Server.h"

using namespace std;

int Server::m_signum = 0;
boost::thread_group Server::m_threads;

Server::Server(const string& name, const string& version) :
m_optionsOK(false),
m_name(name),
m_version(version),
m_pidPath("/var/run/ea-engine.pid"),
m_session()
{
    m_signum = 0;
    signalsHandler();
}

Server::~Server()
{
}	

void Server::setServerConfiguration(int argc, char **argv)
{
    if (conf.readProgramOptions(argc, argv))
    {
        m_optionsOK = true;
    }
}

bool Server::start()
{
    bool res = false;

    if (m_optionsOK)
    {
        log("info") << "[origin enterpriseId=\"40311\" software=\"" << m_name << "\" swVersion=\"" << m_version << "\"] (re)start";

#ifdef NDEBUG
        // Daemonization
        if (chdir("/") != 0)
        {
            cerr << "failed to reach root \n";
            return res;
        }
        if (fork() != 0)
            exit(EXIT_SUCCESS);
        setsid();
        if (fork() != 0)
            exit(EXIT_SUCCESS);

        // Pid File creation
        ofstream pidFile(m_pidPath.c_str());
        if (!pidFile)
        {
            log("error") << m_pidPath << ": " << strerror(errno);
        }
        else
        {
            pidFile << getpid() << std::endl;
        }
#endif

        if (conf.readConfFile())
        {
            m_session.initConnection(conf.getSessConnectParams());
            
            if (conf.isInDB(m_session))
            {
                if (conf.isCleaner())
                {
                    m_threads.create_thread(boost::bind(&Server::removeOldValues, this));
                }
                else
                {	
                    log("info") << "Mode Cleaner disable";
                }
                if (conf.isAlerter())
                {
                    m_threads.create_thread(boost::bind(&Server::checkNewAlerts, this));
                }
                else
                {
                    log("info") << "Mode Alerter disable";
                }
                if (conf.isCalculator())
                {
                    m_threads.create_thread(boost::bind(&Server::calculate, this));
                }
                else
                {
                    log("info") << "Mode Calculator disable";
                }

                res = true;
            }
            else
            {
                log("error") << "This Engine ID is not in the database.";
            }
        }
    }
    return res;
}

int Server::waitForShutdown()
{
    // wait the end of the created thread
    m_threads.join_all();

    return m_signum;
}

void Server::stop()
{
#ifdef NDEBUG
    // Pid File suppression
    if (remove(m_pidPath.c_str()) < 0)
    {
        log("error") << m_pidPath << ": " << strerror(errno);
    }
#endif
    log("info") << "[origin enterpriseId=\"40311\" software=\"" << m_name << "\" swVersion=\"" << m_version << "\"] stop";
}

void Server::restart(int argc, char **argv, char **envp)
{
  char *path = realpath(argv[0], 0);

  // Try a few times since this may fail because we have an incomplete
  // binary...
  for (int i = 0; i < 5; ++i)
  {
    int result = execve(path, argv, envp);
    if (result != 0)
    {
      sleep(1);
    }
  }

  perror("execve");
}

void Server::signalsHandler()
{
    set<unsigned short> ignored {SIGCHLD, SIGCLD, SIGIO, SIGPOLL, SIGSTOP, SIGTSTP, SIGCONT, SIGTTIN, SIGTTOU, SIGURG, SIGWINCH};
    
    for (unsigned short i = 1; i < SIGSYS; i++)
    {
        if (ignored.find(i) == ignored.end())
        {
            signal(i, Server::signalHandling);
        }
    }
}

void Server::signalHandling(int signum)
{    
    m_signum = signum;
    m_threads.interrupt_all();
}

void Server::checkNewAlerts()
{
    AlertProcessor alertProcessor(m_session);
    log("info") << "Mode Alerter start";

    alertProcessor.verifyAlerts(&m_signum);
}

void Server::removeOldValues()
{
    log("info") << "Mode Cleaner start";

    while (m_signum == 0)
    {
        log("info") << "Cleaning of IVA Table";
        
        //remove values older than 1 day from information_value (duplicated in T_INFORMATION_HISTORICAL_VALUE_IHV)
        try
        {
            Wt::Dbo::Transaction transaction(m_session, true);
            const string queryString = "DELETE FROM \"T_INFORMATION_VALUE_IVA\""
                    " WHERE"
                    " \"IVA_CREA_DATE\" < (NOW() - interval '1 day')";
            m_session.execute(queryString);
            transaction.commit();
        }
        catch (Wt::Dbo::Exception const& e)
        {
            log("error") << "Wt::Dbo: " << e.what();
        }
        boost::this_thread::sleep(boost::posix_time::seconds(conf.sleepThreadRemoveOldValues));
    }

    log("info") << "Mode Cleaner stop";
}

void Server::calculate()
{
    const int ivaListSize = 50;

    log("info") << "Mode Calculator start";

    while (m_signum == 0)
    {
        vector<long long> ivaIdList;

        try
        {
            Wt::Dbo::Transaction transaction(m_session, true);
            // we get iva values where state = ToBeCalculate
            string queryString = "SELECT iva FROM \"T_INFORMATION_VALUE_IVA\"  iva"
                    " WHERE \"IVA_STATE\" = 9 FOR UPDATE LIMIT ?";
            Wt::Dbo::collection<Wt::Dbo::ptr<Echoes::Dbo::InformationValue>> ivaList = m_session.query<Wt::Dbo::ptr<Echoes::Dbo::InformationValue>>(queryString).bind(ivaListSize);

            for (Wt::Dbo::collection<Wt::Dbo::ptr<Echoes::Dbo::InformationValue> >::const_iterator it = ivaList.begin(); it != ivaList.end(); ++it)
            {
                m_session.execute("UPDATE \"T_INFORMATION_VALUE_IVA\" SET \"IVA_STATE\" = ? WHERE \"IVA_ID\" = ?").bind(1).bind(it->id());
                ivaIdList.push_back(it->id());
            }

            log("debug") << ivaIdList.size() << " IVA retrieved to calculate";
            transaction.commit();
        }
        catch (Wt::Dbo::Exception const& e)
        {
            log("error") << "Wt::Dbo: " << e.what();
        }

        for (unsigned short i(0); i < ivaIdList.size(); ++i)
        {
            int ivaLotNum, ivaLineNum;
            long long ivaAssetId;
            long long ivaId;

            Wt::WString calculate, realCalculate;

            // we get the information related to the iva into ptrInfTmp ptr
            Wt::Dbo::ptr<Echoes::Dbo::InformationValue> ivaPtr;
            try
            {
                Wt::Dbo::Transaction transaction(m_session, true);
                ivaPtr = m_session.find<Echoes::Dbo::InformationValue>()
                        .where("\"IVA_ID\" = ?").bind(ivaIdList[i])
                        .limit(1);

                ivaLotNum = ivaPtr->lotNumber;
                ivaLineNum = ivaPtr->lineNumber;
                ivaAssetId = ivaPtr->informationData->asset.id();
                ivaId = ivaIdList[i];



                if (ivaPtr->informationData->information->calculate)
                {
                    if (!ivaPtr->informationData->information->calculate.get().empty())
                    {
                        calculate = ivaPtr->informationData->information->calculate.get();
                    }
                }
                transaction.commit();
            }
            catch (Wt::Dbo::Exception const& e)
            {
                log("error") << "Wt::Dbo: " << e.what();
                continue;
            }
            
            if(calculate != "")
            {
                //FIXME
                log("debug") << "calculate value: " << calculate;

                if (calculate == "searchValueToCalculate")
                {
                    //we get the calculation data
                    try
                    {
                        Wt::Dbo::Transaction transaction(m_session, true);

                        Wt::Dbo::ptr<Echoes::Dbo::InformationData> ptrInfData = m_session.find<Echoes::Dbo::InformationData>()
                                .where("\"IDA_FIL_FIL_ID\" = ?").bind(ivaPtr->informationData->filter.id())
                                .where("\"IDA_AST_AST_ID\" = ?").bind(ivaPtr->informationData->asset.id())
                                .where("\"IDA_FILTER_FIELD_INDEX\" = ?").bind(-1)
                                .limit(1);

                        if (ptrInfData->information->calculate)
                        {
                            if (!ptrInfData->information->calculate.get().empty())
                            {
                                realCalculate = ptrInfData->information->calculate.get();
                            }
                        }
                        transaction.commit();
                    }
                    catch (Wt::Dbo::Exception const& e)
                    {
                        log("error") << "Wt::Dbo: " << e.what();
                        continue;
                    }
                }
                else
                {
                    realCalculate = calculate;
                }

                if(realCalculate != "")
                {
                    //calcul
                    try
                    {
                        Wt::Dbo::Transaction transaction(m_session, true);
                        log("debug") << "Launch calc query: " << realCalculate;
                        m_session.execute("SELECT " + realCalculate.toUTF8() + "(?, ?, ?, ?, ?, ?, ?)")
                        .bind(ivaPtr->informationData.id()).bind(ivaLotNum)
                        .bind(9) // state
                        .bind(ivaLineNum).bind(ivaAssetId)
                        .bind(10) // limit
                        .bind(ivaId);
                        log("debug") << "calc done.";
                        transaction.commit();
                    }
                    catch (Wt::Dbo::Exception const& e)
                    {
                        log("error") << "Wt::Dbo: " << e.what();
                        continue;
                    }   
                }
                else
                {
                    log("error") << "no real calculate";
                }
            }
            else
            {
                log("debug") << "no calculate";
            }
        }

        boost::this_thread::sleep(boost::posix_time::seconds(conf.sleepThreadCalculate));
    }

    log("info") << "Mode Calculator stop";
}
