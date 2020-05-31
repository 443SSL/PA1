/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;


int main(int argc, char *argv[]){
    srand(time_t(NULL));

    int control = fork();
    if(control == 0){
        char *server_args[] = {"./server", NULL};
        execvp(server_args[0], server_args);
        perror("execvp fork failure");
    } else {

        // creating channel
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

        // this is a demonstration of creating message to write to server
        datamsg *d1 = new datamsg(1, 0.004, 2 );
        chan.cwrite (&d1, sizeof(datamsg));
        // creating double to hold data when read from server
        double data = 0;
        // reading data from server
        chan.cread(&data, sizeof(double));
        //printf("data = %lf", data);
        // deleting pointer
        delete d1;

        // requesting 1000 individual points from the file
        struct timeval start_time;
        struct timeval end_time;

        // getting start time
        gettimeofday(&start_time, NULL);

        string filename = "x1.csv";
        string output_path = string("received/") + filename;
        FILE *f = fopen(output_path.c_str(), "wb");

        
        int patientNum = -1;
        //for(int i = 0; i < sizeof(argv); i++){
        //    cout << "wow" << endl; 
        //}
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
        }
        
    
        // doing file request
        filemsg *file = new filemsg(0, 0);
        string filename1 = "1.csv";
        int request = sizeof(filemsg) + filename1.size() + 1;
        char *buf1 = new char[request];
        memcpy(buf1, file, sizeof(filemsg));
        strcpy(buf1 + sizeof(filemsg), filename1.c_str());
        chan.cwrite(buf1, request);

        __int64_t filesize;
        chan.cread(&filesize, sizeof(__int64_t));
        
        string output_path1 = string("received/") + filename1;
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

        fclose(f);
        fclose(f1);
        delete buf1;
        delete receiver;

        // closing the channel    
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite (&m, sizeof (MESSAGE_TYPE));
    }
}
