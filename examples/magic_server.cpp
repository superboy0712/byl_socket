/*
 * server.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: yulong
 */

#include "../src/tmpl_socket.h"
#include <vector>
#include <iostream>
using bylSocket::Domain;
using bylSocket::Type;
using bylSocket::Options;
using bylSocket::tryForMax;
using bylSocket::tryforever_interval_not_throw;
using bylSocket::Tmpl::BufferedSocket;
using bylSocket::Tmpl::Socket;
using bylSocket::Tmpl::ListenedSocket;
using namespace std;

void worker(Socket<Domain::IP4, Type::STREAM> client) {
    auto bc = BufferedSocket<Domain::IP4, Type::STREAM>(client);
    bc.set_opt(Options::RCVTIMEO, 2);
    bc.set_opt(Options::SNDTIMEO, 2);
    cout << "worker" << std::this_thread::get_id() << " online!" << endl;
    try {
        do {
            tryForMax(5, [&]() {
                bc.recv();
                bc.fsend("I am worker %ld from server\n",
                         std::this_thread::get_id());
            });
        } while (true);
    } catch (std::exception &e) {
       err_report(e.what());
    }
    cout << "worker" << std::this_thread::get_id() << " offline!" << endl;
}


int main() {

    cout << "hello iam server" << endl;
    std::vector<std::thread> vtworker(10);
    tryforever_interval_not_throw("listen",1, 0.2, [&]() {
        auto s = ListenedSocket<Domain::IP4>();
        /**
         * listened socket can be also configured timeout block!!
         */
        while(1) {
            vtworker.push_back(std::thread(worker, s.accept()));
        }
    });
    for(auto &i : vtworker) {
        i.join();
    }
    cout << "hello" << endl; // prints
    sleep(10);
    return 0;
}



