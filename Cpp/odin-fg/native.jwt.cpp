/**
    Copyright 2016-2019 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <odin/fg/native.hpp>
#include <odin/credentials.hpp>
#include <odin/odin.hpp>
#include <odin/user.hpp>

#include <fost/crypto>
#include <fost/log>
#include <fost/insert>


const fg::frame::builtin odin::lib::mint_login_jwt =
        [](fg::frame &stack,
           fg::json::const_iterator pos,
           fg::json::const_iterator end) {
            auto username =
                    stack.resolve_string(stack.argument("username", pos, end));
            auto password =
                    stack.resolve_string(stack.argument("password", pos, end));
            fostlib::json payload;
            if (pos != end) payload = stack.argument("payload", pos, end);

            auto cnx = connect(stack);
            auto user = odin::credentials(cnx, username, password);
            if (user.isnull())
                throw fostlib::exceptions::not_implemented(
                        __func__,
                        "The user does not appear in the database so no JWT "
                        "can be minted");

            auto token = odin::mint_login_jwt(user, std::move(payload))
                                 .token(odin::c_jwt_secret.value().data());
            stack.symbols["odin.jwt"] = token;

            auto headers = stack.symbols["testserver.headers"];
            fostlib::jcursor("Authorization")
                    .set(headers, fg::json("Bearer " + token));
            stack.symbols["testserver.headers"] = headers;

            return fg::json(std::move(token));
        };


const fg::frame::builtin odin::lib::mint_jwt =
        [](fg::frame &stack,
           fg::json::const_iterator pos,
           fg::json::const_iterator end) -> fg::json {
    auto payload = stack.resolve(stack.argument("payload", pos, end));
    fostlib::string secret;
    if (pos == end) {
        secret = odin::c_jwt_secret.value();
    } else {
        secret = stack.resolve_string(stack.argument("secret", pos, end));
    }
    return fg::json{
            fostlib::jwt::mint{fostlib::jwt::alg::HS256, std::move(payload)}
                    .token(secret.data())};
};


const fg::frame::builtin odin::lib::jwt_payload =
        [](fg::frame &stack,
           fg::json::const_iterator pos,
           fg::json::const_iterator end) {
            fostlib::string jwt;
            if (pos != end) {
                jwt = stack.resolve_string(stack.argument("jwt", pos, end));
            } else {
                jwt = fostlib::coerce<fostlib::string>(
                        stack.lookup("odin.jwt"));
            }
            auto secret = odin::c_jwt_secret.value();
            if (pos != end) {
                auto app_id = stack.resolve_string(
                        stack.argument("app_id", pos, end));
                secret = odin::c_jwt_secret.value() + app_id;
            }
            auto token = fostlib::jwt::token::load(secret, jwt);
            if (not token) {
                return fostlib::json();
            } else {
                return token.value().payload;
            }
        };

const fg::frame::builtin odin::lib::mint_reset_password_jwt =
        [](fg::frame &stack,
           fg::json::const_iterator pos,
           fg::json::const_iterator end) {
            auto username =
                    stack.resolve_string(stack.argument("username", pos, end));
            auto cnx = connect(stack);
            if (!odin::does_user_exist(cnx, username))
                throw fostlib::exceptions::not_implemented(
                        __func__,
                        "The user does not appear in the database so no JWT "
                        "can be minted");
            auto token = odin::mint_reset_password_jwt(username).token(
                    odin::c_jwt_reset_forgotten_password_secret.value().data());
            return fg::json(std::move(token));
        };
