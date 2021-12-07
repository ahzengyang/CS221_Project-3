//
//  climate.c
//  Project_3
//
//  Created by Alex Zeng-Yang on 28/11/21.
//
/**
 * Performs analysis on climate data provided by the
 * National Oceanic and Atmospheric Administration (NOAA).
 *
 * Input:    Tab-delimited file(s) to analyze.
 * Output:   Summary information about the data.
 *
 * Compile:  run make
 *
 * Example Run:      ./climate data_tn.tdv data_wa.tdv
 *
 *
 * Opening file: data_tn.tdv
 * Opening file: data_wa.tdv
 * States found: TN WA
 * -- State: TN --
 * Number of Records: 17097
 * Average Humidity: 49.4%
 * Average Temperature: 58.3F
 * Max Temperature: 110.4F
 * Max Temperatuer on: Mon Aug  3 11:00:00 2015
 * Min Temperature: -11.1F
 * Min Temperature on: Fri Feb 20 04:00:00 2015
 * Lightning Strikes: 781
 * Records with Snow Cover: 107
 * Average Cloud Cover: 53.0%
 * -- State: WA --
 * Number of Records: 48357
 * Average Humidity: 61.3%
 * Average Temperature: 52.9F
 * Max Temperature: 125.7F
 * Max Temperature on: Sun Jun 28 17:00:00 2015
 * Min Temperature: -18.7F
 * Min Temperature on: Wed Dec 30 04:00:00 2015
 * Lightning Strikes: 1190
 * Records with Snow Cover: 1383
 * Average Cloud Cover: 54.5%
 *
 * TDV format:
 *
 * CA» 1428300000000»  9prcjqk3yc80»   93.0»   0.0»100.0»  0.0»95644.0»277.58716
 * CA» 1430308800000»  9prc9sgwvw80»   4.0»0.0»100.0»  0.0»99226.0»282.63037
 * CA» 1428559200000»  9prrremmdqxb»   61.0»   0.0»0.0»0.0»102112.0»   285.07513
 * CA» 1428192000000»  9prkzkcdypgz»   57.0»   0.0»100.0»  0.0»101765.0» 285.21332
 * CA» 1428170400000»  9prdd41tbzeb»   73.0»   0.0»22.0»   0.0»102074.0» 285.10425
 * CA» 1429768800000»  9pr60tz83r2p»   38.0»   0.0»0.0»0.0»101679.0»   283.9342
 * CA» 1428127200000»  9prj93myxe80»   98.0»   0.0»100.0»  0.0»102343.0» 285.75
 * CA» 1428408000000»  9pr49b49zs7z»   93.0»   0.0»100.0»  0.0»100645.0» 285.82413
 *
 * Each field is separated by a tab character \t and ends with a newline \n.
 *
 * Fields:
 *      state code (e.g., CA, TX, etc),
 *      timestamp (time of observation as a UNIX timestamp),
 *      geolocation (geohash string),
 *      humidity (0 - 100%),
 *      snow (1 = snow present, 0 = no snow),
 *      cloud cover (0 - 100%),
 *      lightning strikes (1 = lightning strike, 0 = no lightning),
 *      pressure (Pa),
 *      surface temperature (Kelvin)
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_STATES 50

struct climate_info {
    char code[3];
    unsigned long num_records;
    long double sum_humidity;
    long double sum_temperature;
    double max_temperature;
    long max_date;
    double min_temperature;
    long min_date;
    int lightning_strikes;
    int snow;
    long double sum_cloud;
};

void analyze_file(FILE *file, struct climate_info *states[], int num_states);
int getIndex(char *code, struct climate_info **states);
void print_report(struct climate_info *states[], int num_states);

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s tdv_file1 tdv_file2 ... tdv_fileN \n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Let's create an array to store our state data in. As we know, there are
     * 50 US states. */
    struct climate_info *states[NUM_STATES] = { NULL };

    int i;
    for (i = 1; i < argc; ++i) {
        FILE *file = fopen(argv[i], "r");

        if (file == NULL) {
            printf("The file: \"%s\" does not exist.", argv[i]);
            continue;
        }
        
        analyze_file(file, states, NUM_STATES);
        fclose(file);
    }

    /* Now that we have recorded data for each file, we'll summarize them: */
    print_report(states, NUM_STATES);

    return 0;
}

void analyze_file(FILE *file, struct climate_info **states, int num_states) {
    const int line_sz = 100;
    char line[line_sz];
    char delim[2] = "\t";
    char *token;
    struct climate_info *state;
    
    while (fgets(line, line_sz, file) != NULL) {
        // state code
        token = strtok(line, delim);
        int index = getIndex(token, states);
        if (index < 0) {
            state = (struct climate_info*) malloc(sizeof(struct climate_info));
            index = index == -50 ? 0 : -index;
            states[index] = state;
            strcpy(state->code, token);
            state->sum_humidity = 0;
            state->sum_temperature = 0;
            state->max_temperature = 0;
            state->min_temperature = 0;
            state->lightning_strikes = 0;
            state->snow = 0;
            state->sum_cloud = 0;
        }
        state = states[index];
        state->num_records++;
        
        // timestamp
        token = strtok(NULL, delim);
        unsigned long date = atol(token) / 1000; // unix to ctime precision conversion
        
        // geolocation; unused
        token = strtok(NULL, delim);
        
        // humidity
        token = strtok(NULL, delim);
        state->sum_humidity += atof(token);
        
        // snow
        token = strtok(NULL, delim);
        state->snow += atoi(token);
        
        // cloud cover
        token = strtok(NULL, delim);
        state->sum_cloud += atof(token);
        
        // lightning strikes
        token = strtok(NULL, delim);
        state->lightning_strikes += atoi(token);
        
        // pressure; unused
        token = strtok(NULL, delim);
        
        // surface temperature
        token = strtok(NULL, delim);
        double tempF = atof(token) * 1.8 - 459.67;
        state->sum_temperature += tempF;
        if (tempF > state->max_temperature) { // records first max; not latest; use >= for other way
            state->max_temperature = tempF;
            state->max_date = date;
        }
        if (tempF < state->max_temperature) {
            state->min_temperature = tempF;
            state->min_date = date;
        }
    }
}

int getIndex(char *code, struct climate_info **states) {
    int i;
    for (i = 0; *states != NULL; i++) {
        if (strcmp(states[i]->code, code))
            return i;
    }
    if (i == 0)
        return -50;
    return -i;
}

void print_report(struct climate_info *states[], int num_states) {
    printf("States found:\n");
    int i;
    for (i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
            struct climate_info *info = states[i];
            unsigned long num_rec = info->num_records;
            printf("-- State: %s --\n", info->code);
            printf("Number of Records: %lu\n", num_rec);
            printf("Average Humidity: %Lf\n", info->sum_humidity/num_rec);
            printf("Average Temperature: %Lf\n", info->sum_temperature/num_rec);
            printf("Max Temperature: %lf\n", info->max_temperature);
            printf("Max Temperature on: %s\n", ctime(&info->max_date));
            printf("Min Temperature: %lf\n", info->min_temperature);
            printf("Min Temperature on: %s\n", ctime(&info->min_date));
            printf("Lightning Strikes: %d\n", info->lightning_strikes);
            printf("Records with Snow Cover: %d\n", info->snow);
            printf("Average Cloud Cover: %Lf\n", info->sum_cloud/num_rec);
        }
    }
    printf("\n");
}
