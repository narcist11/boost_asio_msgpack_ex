#include "message.h"

#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/version.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include <stdint.h>
#include <stdlib.h>

using boost::asio::ip::tcp;

void handle_write(const boost::system::error_code& /* error */,
                  size_t /* bytes_transferred */)
{
}

int main(int argc, char *argv[])
{
    try {
        if (argc != 6) {
            std::cerr << "Usage: mtc host port name id gpa" << std::endl;
            return EXIT_FAILURE;
        }

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[1], argv[2]);

        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        tcp::socket s(io_service);

#if ((BOOST_VERSION / 100 % 1000) >= 47)
        boost::asio::connect(s, endpoint_iterator);
#else
#warning "minor version of boost is < 47"
        tcp::resolver::iterator end;
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
            s.close();
            s.connect(*endpoint_iterator++, error);
        }
        if (error)
            throw boost::system::system_error(error);
#endif

        // send student record
        int id = strtol(argv[4], NULL, 10);
        double gpa = strtod(argv[5], NULL);
        student_record sr(std::string(argv[3]), id, gpa);

        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, sr);

        int32_t smsglen = sbuf.size();
        char *smsg = new char [sizeof(smsglen) + smsglen];
        memcpy(smsg, &smsglen, sizeof(smsglen));
        memcpy(smsg + sizeof(smsglen), sbuf.data(), smsglen);

        boost::asio::async_write(s,
                boost::asio::buffer(smsg, (sizeof(smsglen) + smsglen)),
                boost::bind(&handle_write,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)
                );

        delete [] smsg;

        io_service.run();

        // receive evaluation

        // read length
        size_t len;
        int32_t msglen;
        boost::system::error_code ec;

        len = s.read(
                boost::asio::buffer(&msglen, sizeof(msglen)),
                ec);
        if (ec == boost::asio::error::eof) {
            return 0; // Coonection closed cleanly by peer.
        } else if (ec) {
            throw boost::system::system_error(ec);
        }

        char *msg = new char[msglen];

        len = s.read(
                boost::asio::buffer(msg, msglen),
                ec);
        if (ec == boost::asio::error::eof) {
            return 0; // Coonection closed cleanly by peer.
        } else if (ec) {
            throw boost::system::system_error(ec);
        }

        msgpack::unpacked unpacked_msg;
        msgpack::unpack(&unpacked_msg, msg, msglen);

        msgpack::object obj = unpacked_msg.get();

        evaluation eval;
        obj.convert(&eval);

        std::cout << eval.get_remark() <<
            ", result = " <<
            (eval.get_passed() ? "Passed!" : "Failure!") <<
            std::endl;

        delete [] msg;

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
