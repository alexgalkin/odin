/*
    Copyright 2016 Felspar Co Ltd. http://odin.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include <odin/odin.hpp>
#include <odin/views.hpp>
#include <odin/credentials.hpp>
#include <fost/crypto>
#include <fost/test>
#include <fost/exception/parse_error.hpp>


FSL_TEST_SUITE(credentials);

namespace {
    fostlib::json configuration() {
        fostlib::json config;
        fostlib::insert(config, "expires", "hours", 72);
        return config;
    }

    const fostlib::timestamp c_epoch(1970, 1, 1);
}

FSL_TEST_FUNCTION(check_can_renew_jwt_with_no_data_loss_and_add_exp) {

    fostlib::json payload;
    const fostlib::jcursor iss("iss");
    fostlib::insert(payload, iss, "http://odin.felspar.com/app/app01");
    const fostlib::jcursor sub("sub");
    fostlib::insert(payload, sub, "user01");
    const fostlib::jcursor email("email");
    fostlib::insert(payload, email, "test@email.com");

    fostlib::jwt::mint jwt(fostlib::jwt::alg::HS256, payload);
    fostlib::string secret = odin::c_jwt_secret.value();
    std::string jwt_token = jwt.token(odin::c_jwt_secret.value().data());

    auto const result = odin::renew_jwt(jwt_token, secret, configuration());

    auto load_jwt = fostlib::jwt::token::load(secret, result.first);
    FSL_CHECK(load_jwt.has_value());
    FSL_CHECK_EQ(load_jwt->payload[iss], payload[iss]);
    FSL_CHECK_EQ(load_jwt->payload[sub], payload[sub]);
    FSL_CHECK_EQ(load_jwt->payload[email], payload[email]);
    const fostlib::jcursor exp("exp");
    FSL_CHECK(load_jwt->payload.has_key(exp));

}

FSL_TEST_FUNCTION(check_can_renew_jwt_with_no_data_loss_and_extend_exp) {
    
    fostlib::json payload;
    const fostlib::jcursor iss("iss");
    fostlib::insert(payload, iss, "http://odin.felspar.com/app/app01");
    const fostlib::jcursor sub("sub");
    fostlib::insert(payload, sub, "user01");
    auto config = configuration();
    fostlib::json old_exp_config;
    fostlib::insert(old_exp_config, "expires", "hours", 24);
    const auto old_exp = fostlib::timestamp::now() + fostlib::coerce<fostlib::timediff>(old_exp_config["expires"]);
    const auto old_exp_timestamp = (old_exp - c_epoch).total_seconds();
    const fostlib::jcursor exp("exp");
    fostlib::insert(payload, exp, old_exp_timestamp);

    fostlib::jwt::mint jwt(fostlib::jwt::alg::HS256, payload);
    fostlib::string secret = odin::c_jwt_secret.value();

    std::string jwt_token = jwt.token(odin::c_jwt_secret.value().data());

    auto const result = odin::renew_jwt(jwt_token, secret, configuration());
    auto load_jwt = fostlib::jwt::token::load(secret, result.first);
    FSL_CHECK(load_jwt.has_value());
    FSL_CHECK_NEQ(load_jwt->payload[exp], payload[exp]);
    FSL_CHECK(result.second > old_exp);

}