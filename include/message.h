#ifndef MESSAGE_H
#define MESSAGE_H

#include "msgpack.hpp"
#include <string>

enum message_id {
    STUDENT_RECORD = 1,
    EVALUATION
};

class student_record {
private:
    std::string name_;
    int id_;
    double gpa_; // grade point average

public:
    MSGPACK_DEFINE(name_, id_, gpa_);

    student_record(const std::string& name, int id, double gpa):
        name_(name), id_(id), gpa_(gpa)
    {}

    student_record():
        name_("unknown"), id_(), gpa_()
    {}

    void set_name(const std::string& name)
    {
        name_ = name;
    }
    void set_id(int id)
    {
        id_ = id;
    }
    void set_gpa(double gpa)
    {
        gpa_ = gpa;
    }

    std::string get_name() const
    {
        return name_;
    }
    int get_id() const
    {
        return id_;
    }
    double get_gpa() const
    {
        return gpa_;
    }
};

class evaluation {
private:
    std::string remark_;
    bool passed_;

public:
    MSGPACK_DEFINE(remark_, passed_);

    evaluation(const std::string& remark, bool passed):
        remark_(remark), passed_(passed)
    {}

    evaluation():
        remark_("i can not parse your record."), passed_(false)
    {}

    void set_remark(const std::string& remark)
    {
        remark_ = remark;
    }
    void set_passed(bool status)
    {
        passed_ = status;
    }

    std::string get_remark() const
    {
        return remark_;
    }
    bool get_passed() const
    {
        return passed_;
    }
};

#endif // !MESSAGE_H
