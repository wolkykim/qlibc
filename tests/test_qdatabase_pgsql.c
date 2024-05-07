/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2024 SunBeau.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
/* This code is written and updated by following people and released under
 * the same license as above qLibc license.
 * Copyright (c) 2015 Zhenjiang Xie - https://github.com/Charles0429
 *****************************************************************************/

#include "libpq-fe.h"

#include "qunit.h"
#include "qlibc.h"
#include "qlibcext.h"

const char *default_dbtype     = "PGSQL";
const char *default_addr       = "127.0.0.1";
const int   default_port       =  5432;
const char *default_database   = "postgres";
const char *default_username   = "postgres";
const char *default_password   = "123456";
const bool  default_autocommit =  true;

QUNIT_START("Test qdatabase_pgsql.c");

qdb_t* test_connect_database(const char *database) {
    if (database == NULL) {
        database = default_database;
    }

    qdb_t *db = qdb(
        default_dbtype,
        default_addr,
        default_port,
        database,
        default_username,
        default_password,
        default_autocommit
    );
    ASSERT_NOT_NULL(db);
    ASSERT_FALSE(db->connected);
    ASSERT_FALSE(db->get_conn_status(db));

    ASSERT_TRUE(db->open(db));
    ASSERT_TRUE(db->connected);
    ASSERT_TRUE(db->get_conn_status(db));

    return db;
}

qdb_t* test_connect_testdb(qdb_t* database) {
    qdb_t *testdb = NULL;
    char query[128];
    const char *test_database = "qdb_testdb";

    /* test execute_update() */
    snprintf(query, sizeof(query), "DROP DATABASE IF EXISTS %s;", test_database);
    ASSERT_EQUAL_INT(database->execute_update(database, query), 0);

    /* create new database */
    snprintf(query, sizeof(query), "CREATE DATABASE %s;", test_database);
    ASSERT_EQUAL_INT(database->execute_update(database, query), 0);

    /* connect "qdb_testdb" database */
    testdb = qdb(
        default_dbtype,
        default_addr,
        default_port,
        test_database,
        default_username,
        default_password,
        default_autocommit
    );
    ASSERT_NOT_NULL(testdb);
    ASSERT_FALSE(testdb->connected);

    ASSERT_TRUE(testdb->open(testdb));
    ASSERT_TRUE(testdb->connected);

    return testdb;
}

/*
    CREATE TABLE animals (
        animal_id INT PRIMARY KEY,
        animal_name VARCHAR(255),
        species VARCHAR(255),
        gender VARCHAR(255),
        age INT,
        weight INT,
        description VARCHAR(255)
    );

    INSERT INTO animals (animal_id, animal_name, species, gender, age, weight, description)
    VALUES
        (1, 'Dog', 'Canis lupus familiaris', 'Male', 2, 15, 'A friendly dog named Max'),
        (2, 'Cat', 'Felis catus', 'Female', 5, 8, 'A lazy cat named Lucy'),
        (3, 'Rabbit', 'Oryctolagus cuniculus', 'Male', 1, 2, 'A cute little rabbit named Bugs'),
        (4, 'Hamster', 'Mesocricetus auratus', 'Female', 2, 50, 'A hyperactive hamster named Sparky'),
        (5, 'Fish', 'Poecilia reticulata', 'Female', 1, 0.1, 'A peaceful betta fish named Blue');
*/
void test_create_table_animals(qdb_t* database) {
    int rows;

    rows = database->execute_update(database,
        "CREATE TABLE animals ("
        "animal_id INT PRIMARY KEY,"
        "animal_name VARCHAR(255),"
        "species VARCHAR(255),"
        "gender VARCHAR(255),"
        "age INT,"
        "weight INT,"
        "description VARCHAR(255)"
        ");"
    );
    ASSERT_EQUAL_INT(rows, 0);

    rows = database->execute_updatef(database,
        "INSERT INTO animals VALUES"
        "(%s, %s, %s, %s, %s, %s, %s),"
        "(%s, %s, %s, %s, %s, %s, %s),"
        "(%s, %s, %s, %s, %s, %s, %s),"
        "(%s, %s, %s, %s, %s, %s, %s),"
        "(%s, %s, %s, %s, %s, %s, %s);",
            "1", "'Dog'",     "'Canis lupus familiaris'",   "'Male'",   "2", "15",  "'A friendly dog named Max'",
            "2", "'Cat'",     "'Felis catus'",              "'Female'", "5", "8",   "'A lazy cat named Lucy'",
            "3", "'Rabbit'",  "'Oryctolagus cuniculus'",    "'Male'",   "1", "2",   "'A cute little rabbit named Bugs'",
            "4", "'Hamster'", "'Mesocricetus auratus'",     "'Female'", "2", "50",  "'A hyperactive hamster named Sparky'",
            "5", "'Fish'",    "'Poecilia reticulata'",      "'Female'", "1", "0.1", "'A peaceful betta fish named Blue'");
    ASSERT_EQUAL_INT(rows, 5);
}

TEST("Test1: qdb()/open()/close()/free()") {
    qdb_t *db = NULL;

    /* test qdb() */
    db = qdb(
        default_dbtype,
        default_addr,
        default_port,
        default_database,
        default_username,
        default_password,
        default_autocommit
    );

    ASSERT_NOT_NULL(db);
    ASSERT_FALSE(db->connected);
    ASSERT_TRUE(db->open(db));
    ASSERT_TRUE(db->connected);
    ASSERT_TRUE(db->close(db));
    ASSERT_FALSE(db->connected);
    db->free(db);
}

TEST("Test2: execute_updatef()") {
    qdb_t *default_db = test_connect_database(default_database);
    ASSERT_TRUE(default_db->connected);
    qdb_t *test_db = test_connect_testdb(default_db);

    /* test: execute_updatef */
    test_create_table_animals(test_db);

    /* disconnect */
    ASSERT_TRUE(test_db->close(test_db));
    ASSERT_TRUE(default_db->close(default_db));

    /* free */
    test_db->free(test_db);
    default_db->free(default_db);
}

TEST("Test3: execute_queryf()") {
    qdb_t *default_db = test_connect_database(default_database);
    ASSERT_TRUE(default_db->connected);
    qdb_t *test_db = test_connect_testdb(default_db);
    test_create_table_animals(test_db);

    /* test: execute_queryf  */
    qdbresult_t *qrst = test_db->execute_queryf(test_db, "SELECT * FROM %s;", "animals");
    ASSERT_EQUAL_INT(qrst->get_rows(qrst), 5);
    ASSERT_EQUAL_INT(qrst->get_cols(qrst), 7);
    ASSERT_EQUAL_INT(qrst->get_row(qrst),  0);
    ASSERT_EQUAL_STR(qrst->get_str(qrst, "animal_name"), "Dog");
    ASSERT_EQUAL_STR(qrst->get_str_at(qrst, 3), "Canis lupus familiaris");
    ASSERT_TRUE(qrst->get_next(qrst));
    ASSERT_EQUAL_INT(qrst->get_row(qrst),  1);
    ASSERT_TRUE(qrst->get_next(qrst));
    ASSERT_EQUAL_INT(qrst->get_row(qrst),  2);

    ASSERT_EQUAL_INT(qrst->get_int(qrst, "animal_id"), 3);
    ASSERT_EQUAL_INT(qrst->get_int_at(qrst, 5), 1);
    qrst->free(qrst);

    /* disconnect */
    ASSERT_TRUE(test_db->close(test_db));
    ASSERT_TRUE(default_db->close(default_db));

    /* free */
    test_db->free(test_db);
    default_db->free(default_db);
}

TEST("Test4: transaction") {
    qdb_t *default_db = test_connect_database(default_database);
    ASSERT_TRUE(default_db->connected);
    qdb_t *test_db = test_connect_testdb(default_db);
    test_create_table_animals(test_db);
    int rows;
    qdbresult_t *qrst;

    /* test: rollback */
    ASSERT_TRUE(test_db->begin_tran(test_db));
    rows = test_db->execute_update(test_db, "DELETE FROM animals WHERE animal_id > 3;");
    ASSERT_EQUAL_INT(rows, 2);
    qrst = test_db->execute_queryf(test_db, "SELECT * FROM %s;", "animals");
    ASSERT_EQUAL_INT(qrst->get_rows(qrst), 3);
    qrst->free(qrst);
    ASSERT_TRUE(test_db->rollback(test_db)); /* rollback */

    /* test: commit */
    ASSERT_TRUE(test_db->begin_tran(test_db));
    qrst = test_db->execute_queryf(test_db, "SELECT * FROM %s;", "animals");
    ASSERT_EQUAL_INT(qrst->get_rows(qrst), 5);
    rows = test_db->execute_update(test_db, "DELETE FROM animals WHERE animal_id > 3;");
    ASSERT_EQUAL_INT(rows, 2);
    ASSERT_TRUE(test_db->commit(test_db)); /* commit */

    qrst = test_db->execute_queryf(test_db, "SELECT * FROM %s;", "animals");
    ASSERT_EQUAL_INT(qrst->get_rows(qrst), 3);
    qrst->free(qrst);

    /* disconnect */
    ASSERT_TRUE(test_db->close(test_db));
    ASSERT_TRUE(default_db->close(default_db));

    /* free */
    test_db->free(test_db);
    default_db->free(default_db);
}

TEST("Test5: autocommit") {
    qdb_t *db = qdb(
        default_dbtype,
        default_addr,
        default_port,
        default_database,
        default_username,
        default_password,
        false
    );

    /* Autocommit set to false is not supported in pgsql */
    ASSERT_NULL(db);
}

TEST("Test6: other") {
    qdb_t *default_db = test_connect_database(default_database);
    ASSERT_TRUE(default_db->connected);
    qdb_t *test_db = test_connect_testdb(default_db);
    test_create_table_animals(test_db);

    /* test:get_error  */
    ASSERT_EQUAL_STR(test_db->get_error(test_db, NULL), "(no error)");
    ASSERT_FALSE(test_db->set_fetchtype(test_db, true));
    ASSERT_EQUAL_STR(test_db->get_error(test_db, NULL), "unsupported operation");

    /* test: ping */
    ASSERT_TRUE(test_db->ping(test_db));

    /* disconnect */
    ASSERT_TRUE(test_db->close(test_db));
    ASSERT_TRUE(default_db->close(default_db));

    /* free */
    test_db->free(test_db);
    default_db->free(default_db);
}

QUNIT_END();