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
    
    // sending a non-sense message, you need to change this
    //char buf [MAX_MESSAGE];
    //char x = 55;
    //chan.cwrite (&x, sizeof (x));
    //int nbytes = chan.cread (buf, MAX_MESSAGE);
    srand(time_t(NULL));

    int control = fork();
    if(control == 0){
        char *server_args[] = {"./server", NULL};
        execvp(server_args[0], server_args);
        perror("execvp fork failure")
    } else {
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
        datamsg *d1 = new datamsg(1, 0.004, 2 );
        chan.cwrite (&d1, sizeof(datamsg));
        double data = 0;
        chan.cread(&data, sizeof(double));
        //printf("data = %lf", data);
        delete d1;

        // doing file request
        filemsg *file = new filemsg(0, 0);
        string filename = "1.csv";
        int request = sizeof(filemsg) + filename.size() + 1;
        char *buf = new char[request];
        memcpy(buf, file, sizeof(filemsg));
        strcpy(buf + sizeof(filemsg), filename.c_str());
        chan.cwrite(buf, request);

        __int64_t filesize;
        chan.cread(&filesize, sizeof(__int64_t));
        
        string output_path = string("received/") + filename;
        FILE *f = fopen(output_path.c_str(), "wb");
        char *receiver = new char[MAX_MESSAGE];

        while(filesize > 0){
            int r_length = min((__int64_t)MAX_MESSAGE, filesize);
            ((filemsg *)buf)->length = r_length;
            chan.cwrite(buf, request);
            chan.cread(receiver, r_length);
            fwrite(receiver, 1, r_length, f);
            ((filemsg *)buf)->offset += r_length;
            filesize -= r_length;
        }

        fclose(f);
        delete buf;
        delete receiver;

        // closing the channel    
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite (&m, sizeof (MESSAGE_TYPE));
    }
}
