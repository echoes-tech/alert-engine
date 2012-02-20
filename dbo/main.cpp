/*
 * Copyright (C) 2010 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */


#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Postgres>
#include <PostgresConnector.h>
#include <User.h>

namespace odb = Wt::Dbo;
using namespace std;



void run()
{
    /*****
     * Dbo tutorial section 3. A first session
     *****/


    PostgresConnector *pc = new PostgresConnector("echoes","echoes","127.0.0.1","5432","toto");

    odb::Session *session = pc->getSession();

    string classShortName = "_USER_";
    string className = User::TABLE_PREFIX.c_str()+classShortName+User::getTrigram().c_str();

    session->mapClass<User>(className.c_str());




    //(*session).dropTables();
    session->dropTables();
    session->createTables();

    {
        odb::Transaction transaction(*session);

        User *user = new User("Joe");

        user->test();

        user->role = User::Visitor;

        odb::ptr<User> userPtr = session->add(user);

        transaction.commit();
    }

    /*****
     * Dbo tutorial section 4. Querying objects
     *****/
    /*
    {
        odb::Transaction transaction(session);

        odb::ptr<User> joe = session.find<User>().where("name = ?").bind("Joe");

        std::cerr << "Joe has karma: " << joe->karma << std::endl;

        odb::ptr<User> joe2 = session.query< odb::ptr<User> >
                              ("select u from \"T_USER_USR\" u").where("name = ?").bind("Joe");

        transaction.commit();
    }

    {
        odb::Transaction transaction(session);

        typedef odb::collection< odb::ptr<User> > Users;

        Users users = session.find<User>();

        std::cerr << "We have " << users.size() << " users:" << std::endl;

        for (Users::const_iterator i = users.begin(); i != users.end(); ++i)
            std::cerr << " T_USER_USR " << (*i)->name
                      << " with karma of " << (*i)->karma << std::endl;

        transaction.commit();
    }
    */
    /*****
     * Dbo tutorial section 5. Updating objects
     *****/
    /*
    {
        odb::Transaction transaction(session);

        odb::ptr<User> joe = session.find<User>().where("name = ?").bind("Joe");

        joe.modify()->karma++;
        joe.modify()->password = "public";

        transaction.commit();
    }

    {
        odb::Transaction transaction(session);
        odb::ptr<User> joe = session.find<User>().where("name = ?").bind("Joe");
        if (joe)
            joe.remove();
        transaction.commit();
    }

    {
        odb::Transaction transaction(session);

        odb::ptr<User> silly = session.add(new User("Toto"));
        silly.modify()->name = "Silly";
        silly.remove();

        transaction.commit();
    }
    */
}

int main(int argc, char **argv)
{
    run();
}
