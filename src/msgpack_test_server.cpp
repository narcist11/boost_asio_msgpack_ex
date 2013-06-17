#include "message.h"

#include "boost/asio.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/bind.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include <stdint.h>

using boost::asio::ip::tcp;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    ~tcp_connection()
    {
        if (incoming_buf_) {
            delete [] incoming_buf_;
        }

        if (outgoing_buf_) {
            delete [] outgoing_buf_;
        }
    }

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        boost::asio::async_read(socket_,
                boost::asio::buffer(&msglen_, sizeof(msglen_)),
                boost::bind(&tcp_connection::handle_msg_length,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)
                );
    }

private:
    tcp_connection(boost::asio::io_service& io_service):
        socket_(io_service), msglen_(),
        incoming_buf_size_(1024), outgoing_buf_size_(1024),
        incoming_buf_(), outgoing_buf_()
    {
        incoming_buf_ = new char[incoming_buf_size_];
        outgoing_buf_ = new char[outgoing_buf_size_];
    }

    void ensure_incoming_buf_capa(size_t required_size)
    {
        size_t orig_size = incoming_buf_size_;
        while (incoming_buf_size_ < required_size) {
            incoming_buf_size_ *= 2;
        }

        if (orig_size != incoming_buf_size_) {
            delete [] incoming_buf_;
            incoming_buf_ = new char [incoming_buf_size_];
        }
    }

    void ensure_outgoing_buf_capa(size_t required_size)
    {
        size_t orig_size = outgoing_buf_size_;
        while (outgoing_buf_size_ < required_size) {
            outgoing_buf_size_ *= 2;
        }

        if (orig_size != outgoing_buf_size_) {
            delete [] outgoing_buf_;
            outgoing_buf_ = new char [outgoing_buf_size_];
        }
    }

    void handle_write(const boost::system::error_code& error,
                      size_t bytes_transferred)
    {
        if (!error) {
            std::cout << "transferred " << bytes_transferred <<
                " byte(s).\n";
        }
    }

    void dispatch_student_record(msgpack::object const& obj)
    {
        student_record sr;
        obj.convert(&sr);

        evaluation eval = make_evaluation(sr);

        msgpack::sbuffer ebuf;
        msgpack::pack(ebuf, eval);

        char *obuf_cusor = outgoing_buf_;
        int32_t omsg_id = EVALUATION;
        int32_t omsglen = sizeof(omsg_id) + ebuf.size();
        size_t olen = sizeof(omsglen) + omsglen;
        ensure_outgoing_buf_capa(olen);

        memcpy(obuf_cusor, &omsglen, sizeof(omsglen));
        obuf_cusor += sizeof(omsglen);
        memcpy(obuf_cusor, &omsg_id, sizeof(omsg_id));
        obuf_cusor += sizeof(omsg_id);
        memcpy(obuf_cusor, ebuf.data(), ebuf.size());

        boost::asio::async_write(socket_,
                boost::asio::buffer(outgoing_buf_, olen),
                boost::bind(&tcp_connection::handle_write,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)
                );
    }

    //
    // message frame
    //
    // +--------+------------+--------------+
    // | length | message id | message body |
    // +--------+------------+--------------+
    //

    void handle_msg_length(const boost::system::error_code& error,
            size_t bytes_transferred)
    {
        if (!error) {
            ensure_incoming_buf_capa(msglen_);
            boost::asio::async_read(socket_,
                    boost::asio::buffer(incoming_buf_, msglen_),
                    boost::bind(&tcp_connection::handle_msg_body,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
                    );
        } else if (error == boost::asio::error::eof) {
            return; // Connection closed cleanly by peer.
        } else {
            throw boost::system::system_error(error);
        }
    }

    void handle_msg_body(const boost::system::error_code& error,
            size_t bytes_transferred)
    {
        if (!error) {
            bool invalid_msg_id = false;
            int32_t msg_id = 0;
            size_t len = 0;
            char *buf = incoming_buf_;

            memcpy(&msg_id, buf, sizeof(msg_id));
            buf += sizeof(msg_id);
            len = msglen_ - sizeof(msg_id);

            msgpack::unpacked unpacked_msg;
            msgpack::unpack(&unpacked_msg, buf, len);

            msgpack::object obj = unpacked_msg.get();

            // dispatch messages
            switch (msg_id) {
            case STUDENT_RECORD:
                dispatch_student_record(obj);
                break;
            default:
                invalid_msg_id = true;
                std::cout << "invalid message id = " << msg_id << std::endl;
                break;
            };

            if (!invalid_msg_id) {
                start(); // handle other messages..
            }
        } else if (error == boost::asio::error::eof) {
            return; // Connection closed cleanly by peer.
        } else {
            throw boost::system::system_error(error);
        }
    }

    evaluation make_evaluation(student_record const& sr)
    {
        std::ostringstream remark;
        evaluation eval;
        double gpa = sr.get_gpa();

        remark << "Dear, " <<
            sr.get_name() << ". (id=" <<
            sr.get_id() << ") (gpa=" <<
            sr.get_gpa() << "): ";

        if (gpa >= 4.0) {
            eval.set_passed(true);
            remark << "Excellent";
        } else if (gpa >= 3.5) {
            eval.set_passed(true);
            remark << "Good";
        } else if (gpa >= 3.0) {
            eval.set_passed(true);
            remark << "Satisfactory";
        } else if (gpa >= 2.5) {
            eval.set_passed(true);
            remark << "Above Average";
        } else if (gpa >= 2.0) {
            eval.set_passed(true);
            remark << "Average";
        } else if (gpa >= 1.5) {
            eval.set_passed(true);
            remark << "Pass";
        } else if (gpa >= 1.0) {
            eval.set_passed(true);
            remark << "Just Pass";
        } else {
            eval.set_passed(false);
            remark << "Fail";
        }

        eval.set_remark(remark.str());
        return eval;
    }

    tcp::socket socket_;
    int32_t msglen_;
    size_t incoming_buf_size_;
    size_t outgoing_buf_size_;
    char *incoming_buf_;
    char *outgoing_buf_;
};

class tcp_server {
public:
    tcp_server(boost::asio::io_service& io_service, int port):
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        tcp_connection::pointer new_connection = 
            tcp_connection::create(acceptor_.get_io_service());
        acceptor_.async_accept(new_connection->socket(),
                boost::bind(&tcp_server::handle_accept, this, new_connection,
                boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection,
            const boost::system::error_code& error)
    {
        if (!error) {
            new_connection->start();
        }

        start_accept();
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: mts port" << std::endl;
            return 0;
        }

        boost::asio::io_service io_service;
        tcp_server server(io_service, atoi(argv[1]));
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
	return 0;
}
