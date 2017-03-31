/*
 * client.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: yulong
 */


#include "../src/tmpl_socket.h"
#include <iostream>
using bylSocket::Domain;
using bylSocket::Type;
using bylSocket::Options;
using bylSocket::tryForMaxInterval;
using bylSocket::Tmpl::BufferedSocket;
using namespace std;

int main() {
    using namespace bylSocket;
    cout << "hello iam client" << endl;
    try {
        BufferedSocket<Domain::IP4, Type::STREAM> s;
        s.set_opt(Options::SNDTIMEO, 1);
        s.set_opt(Options::RCVTIMEO, 1);
        s.set_opt(Options::REUSEADDR);
        s.connect("127.0.0.1", "50000");
        for (int i = 0; i < 20; i++) {
            puts("sending");
            tryForMaxInterval(6, 1, [&]() {
                s.fsend("i am client hoho! %d\n", i);
                puts(s.recv());
            });
            sleep(4);
        }

    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
    }

    cout << "hello" << endl; // prints

    return 0;
}


