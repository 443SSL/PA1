/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>

using namespace std;


int main(int argc, char *argv[]){
    srand(time_t(NULL));

    bool ecgF = false;
    bool fileF = false;
    bool chanF = false;
    bool patientF = false;
    bool timeF = false;
    bool bufM = false;
    char* newBufSz;
    int stat;
    int eRec;
    double time;
    int patientNum = -1;
    string filename = "1.csv";

    // controls control flow by parsing cli input
    int cliArg;
    while((cliArg = getopt(argc, argv, "p:t:e:f:c:m")) != -1){
        switch(cliArg){
            case 'p':
                patientNum = atoi(optarg);
                patientF = true;
                break;
            case 't':
                time = atof(optarg);
                timeF = true;
                break;
            case 'e':
                eRec = atoi(optarg);
                ecgF = true;
                break;
            case 'f':
                filename = optarg;
                fileF = true;
                break;
            case 'c':
                chanF = true;
                break;
            case 'm':
                newBufSz = optarg;
                bufM = true;
            default:
            // no argument case
                break;

        }
    }

    pid_t pid = fork();
    if(pid == 0){
        if(bufM){
            char *server_args[] = {"./server","-m", newBufSz, NULL};
            execvp(server_args[0], server_args);
            perror("execvp fork failure");
        } else {
            char *server_args[] = {"./server", NULL};
            execvp(server_args[0], server_args);
            perror("execvp fork failure");
        }
    }
    // this is a demonstration of creating message to write to server
    //datamsg *d1 = new datamsg(1, 0.004, 2 );
    //chan.cwrite (&d1, sizeof(datamsg));
    // creating double to hold data when read from server
    //double data = 0;
    // reading data from server
    //chan.cread(&data, sizeof(double));
    //printf("data = %lf", data);
    // deleting pointer
    //delete d1;

    if(patientF && timeF && ecgF){
            // creating channel
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
        datamsg dat(patientNum, time, eRec);
        chan.cwrite(&dat, sizeof(dat));

        MESSAGE_TYPE quitting = QUIT_MSG;
        chan.cwrite (&quitting, sizeof (MESSAGE_TYPE));
        

    } else if (patientF && !timeF && !ecgF){

        // creating channel
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
        // requesting 1000 individual points from the file
        struct timeval start_time;
        struct timeval end_time;

        // getting start time
        gettimeofday(&start_time, NULL);

        string output_path = string("received/x") + filename;
        FILE *f = fopen(output_path.c_str(), "wb");

        if(patientNum != -1){
            ofstream request_data_point;
            request_data_point.open("received/x1.csv");
            double x = 0;
            while(x < 59.996){
                datamsg dat1(patientNum, x, 1);
                datamsg dat2(patientNum, x, 2);

                chan.cwrite(&dat1, sizeof(datamsg));
                double data1 = -1;
                chan.cread((char*)& data1, sizeof(double));

                chan.cwrite(&dat2, sizeof(datamsg));
                double data2 = -1;
                chan.cread((char*)& data2, sizeof(double));
                
                x += 0.004;
            }
            request_data_point.close();

            gettimeofday(&end_time, NULL);

            double runtime = (end_time.tv_sec - start_time.tv_sec) * 1e6;
            runtime = (runtime + (end_time.tv_sec - start_time.tv_sec)) * 1e-6;
            cout << "Runtime of copying datapoints of 1.csv :" << fixed << runtime << setprecision(6);
            cout << "sec" << endl;
            fclose(f);
            MESSAGE_TYPE quitting = QUIT_MSG;
            chan.cwrite (&quitting, sizeof (MESSAGE_TYPE));
        }
    } else if(fileF){
        struct timeval start_time;
        struct timeval end_time;

        // getting start time
        gettimeofday(&start_time, NULL);

        // creating channel
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
        // doing file request
        filemsg *file = new filemsg(0, 0);
        int request = sizeof(filemsg) + filename.size() + 1;
        char *buf1 = new char[request];
        memcpy(buf1, file, sizeof(filemsg));
        strcpy(buf1 + sizeof(filemsg), filename.c_str());
        chan.cwrite(buf1, request);

        __int64_t filesize;
        chan.cread(&filesize, sizeof(__int64_t));
        
        string output_path1 = string("received/") + filename;
        FILE *f1 = fopen(output_path1.c_str(), "wb");
        char *receiver = new char[MAX_MESSAGE];

        while(filesize > 0){
            int r_length = min((__int64_t)MAX_MESSAGE, filesize);
            ((filemsg *)buf1)->length = r_length;
            chan.cwrite(buf1, request);
            chan.cread(receiver, r_length);
            fwrite(receiver, 1, r_length, f1);
            ((filemsg *)buf1)->offset += r_length;
            filesize -= r_length;
        }

        
        fclose(f1);
        delete buf1;
        delete receiver;
        // closing the channel    
        MESSAGE_TYPE quitting = QUIT_MSG;
        chan.cwrite (&quitting, sizeof (MESSAGE_TYPE));

        gettimeofday(&end_time, NULL);

        double runtime = (end_time.tv_sec - start_time.tv_sec) * 1e6;
        runtime = (runtime + (end_time.tv_sec - start_time.tv_sec)) * 1e-6;
        cout << "Runtime of copying datapoints of 1.csv :" << fixed << runtime << setprecision(6);
        cout << "sec" << endl;
    } else if(chanF){
        // creating channel
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
        MESSAGE_TYPE quitting = QUIT_MSG;
        chan.cwrite (&quitting, sizeof (MESSAGE_TYPE));
    } else {
        // creating channel
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
        cout << "error" << endl;
        MESSAGE_TYPE quitting = QUIT_MSG;
        chan.cwrite (&quitting, sizeof (MESSAGE_TYPE));
    }

        

    pid_t progExit = wait(&stat);
    if(WIFEXITED(stat)){
        printf("Parent: child exited with status: %d\n", WIFEXITED(stat));
    }
}
