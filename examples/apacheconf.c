/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2014 Seungyoung Kim.
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

#include <stdlib.h>
#include <string.h>
#include "qlibc.h"
#include "qlibcext.h"

#ifdef DISABLE_QACONF
int main(void) {
    printf("qaconf extension is disabled at compile time.\n");
    return 1;
}
#else

// Configuration file to parse.
#define CONF_PATH   "apacheconf.conf"

// Define an example userdata
struct MyConf {
    int listen;         // sample
    int num_hosts;      // sample
};

// Define scope.
//   QAC_SCOPE_ALL and QAC_SCOPE_ROOT are predefined.
//   Custum scope should be defined from 2(1 << 1).
//   Note) These values are ORed(bit operation), so the number should be
//         2(1<<1), 4(1<<2), 6(1<<3), 8(1<<4), ...
enum {
    OPT_SECTION_ALL     = QAC_SECTION_ALL,   // pre-defined
    OPT_SECTION_ROOT    = QAC_SECTION_ROOT,  // pre-defined
    OPT_SECTION_DOMAIN  = (1 << 1),    // user-defined section
    OPT_SECTION_HOST    = (1 << 2),    // user-defined section
};

// Define callback proto-types.
static QAC_CB(confcb_debug);
static QAC_CB(confcb_userdata_example);
static QAC_CB(confcb_section_example);

// Define options and callbacks.
static qaconf_option_t options[] = {
        {"Listen", QAC_TAKE_INT, confcb_userdata_example, 0, OPT_SECTION_ALL},
        {"Protocols", QAC_TAKEALL, confcb_debug, 0, OPT_SECTION_ROOT},
        {"IPSEC", QAC_TAKE_BOOL, confcb_debug, 0, OPT_SECTION_ROOT},
        {"Domain", QAC_TAKE_STR, confcb_debug, OPT_SECTION_DOMAIN, OPT_SECTION_ROOT},
        {  "TTL", QAC_TAKE_INT, confcb_debug, 0, OPT_SECTION_DOMAIN | OPT_SECTION_HOST},
        {  "MX", QAC_TAKE2 | QAC_A1_INT, confcb_debug, 0, OPT_SECTION_DOMAIN},
        {  "Host", QAC_TAKE_STR, confcb_section_example, OPT_SECTION_HOST, OPT_SECTION_DOMAIN},
        {    "IPv4", QAC_TAKE_STR, confcb_debug, 0, OPT_SECTION_HOST},
        {    "TXT", QAC_TAKE_STR, confcb_debug, 0, OPT_SECTION_HOST},
        {    "CNAME", QAC_TAKE_STR, confcb_debug, 0, OPT_SECTION_HOST},
        QAC_OPTION_END
};

int main(void) {
    // Create a userdata structure.
    struct MyConf myconf;
    memset(&myconf, '\0', sizeof(myconf));

    // Initialize and create a qaconf object.
    qaconf_t *conf = qaconf();
    if (conf == NULL) {
        printf("Insufficient memory.\n");
        return -1;
    }

    // Register options.
    conf->addoptions(conf, options);

    // Set callback userdata.
    // This is a userdata which will be provided on callback
    conf->setuserdata(conf, &myconf);

    // Run parser.
    int count = conf->parse(conf, CONF_PATH, QAC_CASEINSENSITIVE);
    if (count < 0) {
        printf("Error: %s\n", conf->errmsg(conf));
    } else {
        printf("Successfully loaded.\n");
    }

    // Verify userdata structure.
    if (conf->errmsg(conf) == NULL) {  // another way to check parsing error.
        printf("\n[Sample MyConf structure]\n");
        printf("MyConf.listen=%d\n", myconf.listen);
        printf("MyConf.num_hosts=%d\n", myconf.num_hosts);
    }

    // Release resources.
    conf->free(conf);

    return 0;
}

static QAC_CB(confcb_debug) {
    int i;
    for (i = 0; i < data->level; i++) {
        printf("    ");
    }

    // Print option name
    if (data->otype == QAC_OTYPE_SECTIONOPEN) {
        printf("<%s>", data->argv[0]);
    } else if (data->otype == QAC_OTYPE_SECTIONCLOSE) {
        printf("</%s>", data->argv[0]);
    } else {  // This is QAC_OTYPE_OPTION type.
        printf("%s", data->argv[0]);
    }

    // Print parent names
    qaconf_cbdata_t *parent;
    for (parent = data->parent; parent != NULL; parent = parent->parent) {
        printf(" ::%s(%s)", parent->argv[0], parent->argv[1]);
    }

    // Print option arguments
    for (i = 1; i < data->argc; i++) {
        printf(" [%d:%s]", i, data->argv[i]);
    }
    printf("\n");

    // Return OK
    return NULL;
}

static QAC_CB(confcb_userdata_example) {
    // Type casting userdata for convenient use.
    struct MyConf *myconf = (struct MyConf *) userdata;

    /*
     * We don't need to verify number of arguments. qaconf() check the number
     * of arguments for us. In this particular case, argc will 2.
     * So the number of arguments is 1 (2-1), since argv[0] is used for
     * option name itself.
     *
     * So below codes are not necessary here, but just to give you an idean
     * how to handle errors.
     */
    //if (data->argc != 2) {
    //    return strdup("Invalid argument!!!");
    //}
    // Copy argument into my structure.
    myconf->listen = atoi(data->argv[1]);

    // Just to print out option information for display purpose.
    confcb_debug(data, userdata);

    // Return OK.
    return NULL;
}

static QAC_CB(confcb_section_example) {
    /*
     * If option is found in brackets like <Option>, that is called section.
     * It has opening section <Option> and closing section </Option>
     *
     * Sometimes, it's easier you handle sections when it's closing.
     * So, for sections, it will be called twice one time at openning and
     * one time at closing because sometimes it's easier you handle sections
     * when it's closing.
     *
     * Section can also take arguments like <Option Arg1 Arg2...>, but only
     * for opening section, because these arguments will be provided at closing
     * callback for you convenient.
     *
     * For example.
     *   <Option Arg1, Arg2> <= Opening section. It can take arguments.
     *                          A callback will be make.
     *
     *   </Option2>          <= Closing section.
     *                          Another callback will be make with arguments
     *                          Arg1 and Arg2.
     *
     * data->otype can be used to determine if it's opening or closing.
     */

    // Just to print out option information for display purpose.
    confcb_debug(data, userdata);

    // This example handles this section at closing.
    if (data->otype != QAC_OTYPE_SECTIONCLOSE) {
        // Bypass QAC_OTYPE_SECTIONOPEN
        return NULL;
    }

    // Type casting userdata for convenient use.
    struct MyConf *myconf = (struct MyConf *) userdata;
    myconf->num_hosts++;

    // Return OK.
    return NULL;
}

#endif
