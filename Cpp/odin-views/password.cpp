/**
    Copyright 2016-2018 Felspar Co Ltd. <http://odin.felspar.com/>

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
*/


#include <odin/credentials.hpp>
#include <odin/nonce.hpp>
#include <odin/odin.hpp>
#include <odin/pwhashproc.hpp>
#include <odin/user.hpp>
#include <odin/views.hpp>

#include <fost/insert>
#include <fost/log>
#include <fostgres/sql.hpp>


namespace {

    const fostlib::module c_odin_reset_forgotten_password(odin::c_odin, "password.cpp");

    std::pair<boost::shared_ptr<fostlib::mime>, int> respond(
        fostlib::string message, int code=403
    ) {
        fostlib::json ret;
        if ( not message.empty() ) fostlib::insert(ret, "message", std::move(message));
        fostlib::mime::mime_headers headers;
        boost::shared_ptr<fostlib::mime> response(
            new fostlib::text_body(fostlib::json::unparse(ret, true), headers, "application/json"));
        return std::make_pair(response, code);
    }


    const class password_me : public fostlib::urlhandler::view {
    public:
        password_me()
        : view("odin.password.me") {
        }

        std::pair<boost::shared_ptr<fostlib::mime>, int> operator () (
            const fostlib::json &config, const fostlib::string &path,
            fostlib::http::server::request &req,
            const fostlib::host &host
        ) const {
            auto body_str = fostlib::coerce<fostlib::string>(
                fostlib::coerce<fostlib::utf8_string>(req.data()->data()));
            fostlib::json body = fostlib::json::parse(body_str);
            fostlib::pg::connection cnx{fostgres::connection(config, req)};
            const auto reference = req.headers()["__odin_reference"].value();
            if ( req.headers().exists("__user") ) {
                const auto username = req.headers()["__user"].value();
                if ( !body.has_key("new-password") || !body.has_key("old-password") ) {
                    return respond("Must supply both old and new password");
                }
                const auto old_password = fostlib::coerce<f5::u8view>(body["old-password"]);
                const auto new_password = fostlib::coerce<f5::u8view>(body["new-password"]);
                auto user = odin::credentials(cnx, username, old_password, req.remote_address());
                cnx.commit();
                if ( user.isnull() ) return respond("Wrong password");
                if ( new_password.bytes() < 8u ) return respond("New password is too short");
                odin::set_password(cnx, reference, username, new_password);
                if ( odin::does_module_enabled(cnx, "opts/logout") )
                    odin::logout_user(cnx, reference, req.headers()["__remote_addr"].value(),
                        username);
                cnx.commit();
                return respond("", 200);
            }
            return respond("No user is logged in", 401);
        }
    } c_password_me;


    const class forgotten_password_reset : public fostlib::urlhandler::view {
    public:
        forgotten_password_reset()
        : view("odin.password.reset-forgotten") {
        }

        std::pair<boost::shared_ptr<fostlib::mime>, int> operator () (
            const fostlib::json &config, const fostlib::string &path,
            fostlib::http::server::request &req,
            const fostlib::host &host
        ) const {
            if ( req.method() != "POST" ) {
                throw fostlib::exceptions::not_implemented(__func__,
                    "Forgotten password reset requires POST. This should be a 405");
            }
            auto body_str = fostlib::coerce<fostlib::string>(
                fostlib::coerce<fostlib::utf8_string>(req.data()->data()));
            fostlib::json body = fostlib::json::parse(body_str);
            fostlib::pg::connection cnx{fostgres::connection(config, req)};
            if ( !body.has_key("reset-password-token") || !body.has_key("new-password") ) {
                return respond("Must supply both reset token and new password");
            }
            const auto reset_token = fostlib::coerce<fostlib::string>(body["reset-password-token"]);
            auto jwt = fostlib::jwt::token::load(
                odin::c_jwt_reset_forgotten_password_secret.value(), reset_token);
            auto username = fostlib::coerce<f5::u8view>(jwt.value().payload["sub"]);
            if( odin::does_user_exist(cnx, username) ){
                const auto reference = odin::reference();
                const auto new_password = fostlib::coerce<f5::u8view>(body["new-password"]);
                odin::set_password(cnx, reference, username, new_password);
                if ( odin::does_module_enabled(cnx, "opts/logout") )
                    odin::logout_user(cnx, reference, req.headers()["__remote_addr"].value(),
                        username);
                cnx.commit();
                return respond("Success", 200);
            }
            throw fostlib::exceptions::not_implemented(__func__, "Invalid reset-password-token.");
        }
    } c_reset_password;


}
