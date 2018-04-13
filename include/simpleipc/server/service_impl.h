#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "../common/connection.h"

namespace simpleipc {
namespace server {

class service_impl {

public:
    class callback_interface {

    public:
        virtual bool connect(connection const& client, nlohmann::json const& req) = 0;

        virtual void disconnect(connection const& client) = 0;

        virtual void handle(connection const& client, nlohmann::json const& req) = 0;

    };

    virtual void set_callback_interface(callback_interface* cb) = 0;

    virtual void bind(std::string const& path) = 0;

    virtual void close() = 0;

};

class service_impl_factory {

public:
    static std::unique_ptr<service_impl> create_platform_service();

};

}
}