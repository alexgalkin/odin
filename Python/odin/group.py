from psycopg2.extras import Json


SET_GROUP = '''INSERT INTO odin.group_ledger
    (reference, group_slug, description)
    VALUES (%s, %s, %s)
    RETURNING *'''


def setgroup(cnx, group, description=''):
    cnx.assert_module('authz')
    cnx.execute(SET_GROUP, (cnx.reference, group, description))
    print(group, "set up")

