
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "version.h"
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "read.h"
#include "write.h"
#include "normal.h"
#include "sort.h"
#include "error.h"
#include "report.h"
#include "allocate.h"

/*
 * Course grade computation program
 */

#define REPORT          0
#define COLLATE         1
#define FREQUENCIES     2
#define QUANTILES       3
#define SUMMARIES       4
#define MOMENTS         5
#define COMPOSITES      6
#define INDIVIDUALS     7
#define HISTOGRAMS      8
#define TABSEP          9
#define ALLOUTPUT      10
#define SORTBY         11
#define NONAMES        12
#define OUTPUT         13

static struct option_info {
        unsigned int val;
        char *name;
        char chr;
        int has_arg;
        char *argname;
        char *descr;
} option_table[] = {
 {REPORT,         "report",    'r',      no_argument, NULL,
                  "Process input data and produce specified reports."},
 {COLLATE,        "collate",   'c',      no_argument, NULL,
                  "Collate input data and dump to standard output."},
 {FREQUENCIES,    "freqs",     0,        no_argument, NULL,
                  "Print frequency tables."},
 {QUANTILES,      "quants",    0,        no_argument, NULL,
                  "Print quantile information."},
 {SUMMARIES,      "summaries", 0,        no_argument, NULL,
                  "Print quantile summaries."},
 {MOMENTS,        "stats",     0,        no_argument, NULL,
                  "Print means and standard deviations."},
 {COMPOSITES,     "comps",     0,        no_argument, NULL,
                  "Print students' composite scores."},
 {INDIVIDUALS,    "indivs",    0,        no_argument, NULL,
                  "Print students' individual scores."},
 {HISTOGRAMS,     "histos",    0,        no_argument, NULL,
                  "Print histograms of assignment scores."},
 {TABSEP,         "tabsep",    0,        no_argument, NULL,
                  "Print tab-separated table of student scores."},
 {ALLOUTPUT,      "all",       'a',      no_argument, NULL,
                  "Print all reports."},
 {SORTBY,         "sortby",    'k',      required_argument, "key",
                  "Sort by {name, id, score}."},
 {NONAMES,        "nonames",   'n',      no_argument, NULL,
                  "Suppress printing of students' names."},
 {OUTPUT,         "output",    'o',      required_argument, "file",
                  "Specify file to be used for output."}
};

static char *short_options = "";
static struct option long_options[15];

static void init_options() {
    short_options = newstring(short_options, 9);
    for(unsigned int i = 0; i < 14; i++) {
        struct option_info *oip = &option_table[i];
        if(oip->val != i) {
            fprintf(stderr, "Option initialization error\n");
            //abort();
        }
        // printf("inside for loop \n");
        struct option *op = &long_options[i];
        op->name = oip->name;
        op->has_arg = oip->has_arg;
        op->flag = NULL;
        op->val = oip->val;
        if (oip->chr != 0) {
            strcat(short_options, &oip->chr);
            // printf("hi \n");
            if(oip->chr == 'k' || oip->chr == 'o') {
                strcat(short_options, ":");
            }
            //printf("passed if \n");
        }
        if (i == 13) {
            op = &long_options[i+1];
            op->name = NULL;
            op->has_arg = 0;
            op->flag = NULL;
            op->val = 0;
        }
    }
}

static int report, collate, freqs, quantiles, summaries, moments,
           scores, composite, histograms, tabsep, nonames, output;

static void usage();

void output_function();

// static int errors, warnings;

int orig_main(argc, argv)
int argc;
char *argv[];
{
        Course *c;
        Stats *s;
        char optval;
        int (*compare)() = comparename;
        FILE *out;

        fprintf(stderr, BANNER);
        init_options();
        if(argc <= 1) usage(argv[0]);
        while(optind < argc) {
            if((optval = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
                //printf("optval is %d \n",optval);
                // free(short_options);
                //if ()
                switch(optval) {
                case REPORT: report++; break;
                case 114: report++; break;
                case COLLATE: collate++; break;
                case 99: collate++; break;
                case TABSEP: tabsep++; break;
                case NONAMES: nonames++; break;
                case 110: nonames++; break;
                case SORTBY:
                    if(!strcmp(optarg, "name"))
                        compare = comparename;
                    else if(!strcmp(optarg, "id"))
                        compare = compareid;
                    else if(!strcmp(optarg, "score"))
                        compare = comparescore;
                    else {
                        fprintf(stderr,
                                "Option '%s' requires argument from {name, id, score}.\n\n",
                                option_table[(int)optval].name);
                        usage(argv[0]);
                    }
                    break;
                case 107:
                    if(!strcmp(optarg, "name"))
                        compare = comparename;
                    else if(!strcmp(optarg, "id"))
                        compare = compareid;
                    else if(!strcmp(optarg, "score"))
                        compare = comparescore;
                    else {
                        fprintf(stderr,
                                "Option '%s' requires argument from {name, id, score}.\n\n",
                                option_table[(int)optval].name);
                        usage(argv[0]);
                    }
                    break;
                case FREQUENCIES: freqs++; break;
                case QUANTILES: quantiles++; break;
                case SUMMARIES: summaries++; break;
                case MOMENTS: moments++; break;
                case COMPOSITES: composite++; break;
                case INDIVIDUALS: scores++; break;
                case HISTOGRAMS: histograms++; break;
                case ALLOUTPUT:
                    freqs++; quantiles++; summaries++; moments++;
                    composite++; scores++; histograms++; tabsep++;
                    break;
                case 97:
                    freqs++; quantiles++; summaries++; moments++;
                    composite++; scores++; histograms++; tabsep++;
                    break;
                case OUTPUT: {
                    if ((out = fopen(optarg, "w")) == NULL) {
                        error("Can't write file: %s\n", optarg);
                        break;
                    }
                    output++;
                    break;
                }
                case 111: {
                    if ((out = fopen(optarg, "w")) == NULL) {
                        error("Can't write file: %s\n", optarg);
                        break;
                    }
                    output++;
                    break;
                }
                case '?':
                    usage(argv[0]);
                    break;
                case ':':
                    usage(argv[0]);
                    break;
                default:
                    break;
                }
            } else {
                break;
            }
        }
        if(optind == argc) {
                fprintf(stderr, "No input file specified.\n\n");
                usage(argv[0]);
        }
        char *ifile = argv[optind];
        if(report == collate) {
                fprintf(stderr, "Exactly one of '%s' or '%s' is required.\n\n",
                        option_table[REPORT].name, option_table[COLLATE].name);
                usage(argv[0]);
        }

        fprintf(stderr, "Reading input data...\n");
        c = readfile(ifile);

        if(errors) {
           printf("%d error%s found, so no computations were performed.\n",
                  errors, errors == 1 ? " was": "s were");
           exit(EXIT_FAILURE);
        }

        fprintf(stderr, "Calculating statistics...\n");
        s = statistics(c);

        if(s == NULL) fatal("There is no data from which to generate reports.");
        normalize(c); //, s);   // reduced parameter
        composites(c);
        sortrosters(c, comparename);
        checkfordups(c->roster);
        if(collate) {
                fprintf(stderr, "Dumping collated data...\n");
                writecourse(stdout, c);
                exit(errors ? EXIT_FAILURE : EXIT_SUCCESS);
        }
        // free(c->number);
        sortrosters(c, compare);
        // free(c->sections);
        fprintf(stderr, "Producing reports...\n");
        if(output) {
            reportparams(out, ifile, c);
            if(moments) reportmoments(out, s);
            if(composite) reportcomposites(out, c, nonames);
            if(freqs) reportfreqs(out, s);
            if(quantiles) reportquantiles(out, s);
            if(summaries) reportquantilesummaries(out, s);
            if(histograms) reporthistos(out, c, s);
            if(scores) reportscores(out, c, nonames);
            if(tabsep) reporttabs(out, c);
        }
        reportparams(stdout, ifile, c);
        if(moments) reportmoments(stdout, s);
        if(composite) reportcomposites(stdout, c, nonames);
        if(freqs) reportfreqs(stdout, s);
        if(quantiles) reportquantiles(stdout, s);
        if(summaries) reportquantilesummaries(stdout, s);
        if(histograms) reporthistos(stdout, c, s);
        if(scores) reportscores(stdout, c, nonames);
        if(tabsep) reporttabs(stdout, c); //, nonames);     //removed nonames

        fprintf(stderr, "\nProcessing complete.\n");
        printf("%d warning%s issued.\n", warnings+errors,
               warnings+errors == 1? " was": "s were");
        exit(errors ? EXIT_FAILURE : EXIT_SUCCESS);
}

void usage(name)
char *name;
{
        struct option_info *opt;

        fprintf(stderr, "Usage: %s [options] <data file>\n", name);
        fprintf(stderr, "Valid options are:\n");
        for(unsigned int i = 0; i < 14; i++) {
                opt = &option_table[i];
                char optchr[5] = {' ', ' ', ' ', ' ', '\0'};
                if(opt->chr)
                  sprintf(optchr, "-%c, ", opt->chr);
                char arg[32];
                if(opt->has_arg)
                    sprintf(arg, " <%.10s>", opt->argname);
                else
                    sprintf(arg, "%.13s", "");
                fprintf(stderr, "\t%s--%-10s%-13s\t%s\n",
                            optchr, opt->name, arg, opt->descr);
                opt++;
        }
        exit(EXIT_FAILURE);
}
