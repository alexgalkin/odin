/**
    Copyright 2016-2019 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <odin/fg/native.hpp>

#include <fost/insert>


const fg::frame::builtin odin::lib::permission =
        [](fg::frame &stack,
           fg::json::const_iterator pos,
           fg::json::const_iterator end) {
            auto cnx = connect(stack);
            fg::json row;
            fostlib::insert(row, "reference", stack.lookup("odin.reference"));
            fostlib::insert(
                    row, "permission_slug",
                    stack.resolve_string(stack.argument("name", pos, end)));
            if (pos != end) {
                fostlib::insert(
                        row, "description",
                        stack.resolve_string(
                                stack.argument("description", pos, end)));
            }
            cnx.insert("odin.permission_ledger", row);
            cnx.commit();
            return fostlib::json();
        };
