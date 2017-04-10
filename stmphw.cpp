#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <regex>
#include <fstream>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <exception>
#include <thread>

using namespace std;

//parsing tools for mail/rcpt
bool mailmatch(string input){
	return(regex_match(input, regex("[Mm][Aa][Ii][Ll] [Ff][Rr][Oo][Mm]:<([A-z0-9]+)@([A-z0-9.]+)>\n")));
}

cmatch capturemail(string input){
	cmatch result;
	regex_match(input.c_str(), result, regex("[Mm][Aa][Ii][Ll] [Ff][Rr][Oo][Mm]:<([A-z0-9]+)@([A-z0-9.]+)>\n"));
	return result;
}

bool mailmatchwtime(string input){
	return(regex_match(input, regex("[Mm][Aa][Ii][Ll] [Ff][Rr][Oo][Mm]:<([A-z0-9]+)@([A-z0-9.]+)> [Tt][Ii][Mm][Ee]:([A-z0-9: ]+)\n")));
}

cmatch capturemailwtime(string input){
	cmatch result;
	regex_match(input.c_str(), result, regex("[Mm][Aa][Ii][Ll] [Ff][Rr][Oo][Mm]:<([A-z0-9]+)@([A-z0-9.]+)> [Tt][Ii][Mm][Ee]:([A-z0-9: ]+)\n"));
	return result;
}

bool rcptmatch(string input){
	return(regex_match(input, regex("[Rr][Cc][Pp][Tt] [Tt][Oo]:<([A-z0-9]+)@([A-z0-9.]+)>\n")));
}

cmatch capturercpt(string input){
	cmatch result;
	regex_match(input.c_str(), result, regex("[Rr][Cc][Pp][Tt] [Tt][Oo]:<([A-z0-9]+)@([A-z0-9.]+)>\n"));
	return result;
}

bool helomatch(string input){
	return(regex_match(input, regex("[Hh][Ee][Ll][Oo] ([A-z.]+)\n")));
}

cmatch capturehelo(string input){
	cmatch result;
	regex_match(input.c_str(), result, regex("[Hh][Ee][Ll][Oo] ([A-z.]+)\n"));
	return result;
}

bool datamatch(string input){
	return(regex_match(input, regex("[Dd][Aa][Tt][Aa]\n")));
}

bool quitmatch(string input){
	return (regex_match(input, regex("[Qq][Uu][Ii][Tt]\n")));
}

//mail class to give a little state to reciever
class mail{
	public:
	 bool helo_done;
     bool from_done;
     bool to_done;
     bool data_progress;
     bool data_done;

     string heloinput;
     string from;
     string time;
     vector<string> to;
     string data;

     mail(){
     	helo_done= false;
     	from_done= false;
     	to_done= false;
     	data_progress = false;
     	data_done= false;


     }

     void clear(){
        helo_done= false;
        from_done= false;
        to_done= false;
        data_progress = false;
        data_done= false;

        heloinput.clear();
        from.clear();
        time.clear();
        to.clear();
        data.clear();
     }

};

string serverresponse(mail& msg, string input){
	if(helomatch(input)){
		if(msg.helo_done){
			return "501";
		}
		else{
			msg.heloinput = capturehelo(input)[1];
			msg.helo_done=true;
			return "250 OK";
		}
	}

	if(mailmatchwtime(input)){
		if(!(msg.helo_done)){
			return "503";
		}

		if(msg.from_done){
			return "501";
		}
		else{
			//set from??
			string temp(capturemailwtime(input)[1]);
			string temp2(capturemailwtime(input)[2]);
			msg.from = temp+"@"+temp2;
			msg.time = capturemailwtime(input)[3];
 			msg.from_done = true;
			return "250 OK";
		}
	}

	if(rcptmatch(input)){
		if(!(msg.helo_done) || !(msg.from_done)){
			return "503";
		}
		else{
			//CHECK IF USER!!!
			string username(capturercpt(input)[1]);
			string domain(capturercpt(input)[2]);
			if(username.compare("jones")==0 && domain.compare("CS353.USC")==0){
				msg.to.push_back(username + "@" + domain);
				msg.to_done = true;
				return "250 OK";
			}
			else{
				if(username.compare("jones")==0){
					return "520 Wrong domain";
				}
				else{
					return "550 No such user here";
				}
			}
			
		}
	}

	if(datamatch(input)){
		if(!(msg.helo_done) || !(msg.from_done) || !(msg.to_done)){
			return "503";
		}
		else{
			return "354 Start mail input; end with <CRLF>.<CRLF>";
		}
	}

	if(quitmatch(input)){
		return "221 CS353.USC service closing transimission channel";
	}

	return "501";

}



int main(int argc, char *argv[]){
	bool amireciever;
	string myfile;
	bool datamode = false; //should be the same criteria so server/client dont disagree
	int portno = 35325;
	string hostname;

	if(argc==1){
		//print instructions
		cout << "smtp -s -r [hostname] [-f filename]\nIt should support the following command line options\n"<<
		"-f, --fname	read and send (or receive and write) the following file.\n" <<
		"-s, --send	operate as a smtp sender.\n" <<
		"-r, --recv	operate as a receiver. Connect to a smtp sender at hostname." << endl;
		return 1;
	}

	//grab command line args and find file & flags

	string cmndline;
	for(int i=0; i<argc; i++){
		cmndline += argv[i];
		cmndline+=" ";
		//grab file name while were 
		if(strcmp(argv[i],"-f")==0){
			if((i+1)>=argc){ 
				cout << "Error inproper command line input" << endl;
				return 1;
			}
			else{
				myfile+=argv[i+1];
			}
		}
	}

	if(cmndline.find(" -s ")!=string::npos){
		amireciever = false;
	}
	else{
		if(cmndline.find(" -r ")!=string::npos){
			amireciever = true;
		}
		else{
			cout << "Error can't be both sender & reciever";
			return 1;
			
		}
	}


	//done w cml & setup
	
	//youre the reciever (aaka server)
	if(amireciever){
		char str[100];
		ofstream file;
		string user = "jones"; //specified as only user on piazza

		//make sure we can open file before anything
		try{
			file.open(myfile);
		}
		catch(exception e){
			cout <<"Error file didn't open" <<endl;
			return 1;
		}

		
		//set up actual recieving
		int listen_nm, comm;
		struct sockaddr_in servaddr;

		listen_nm = socket(AF_INET, SOCK_STREAM, 0);
		bzero(&servaddr, sizeof(servaddr));

		servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    	servaddr.sin_port = htons(portno);

    	bind(listen_nm, (struct sockaddr*) &servaddr, sizeof(servaddr));

		//allows 1 connection
		listen(listen_nm, 1);
		comm = accept(listen_nm, (struct sockaddr*) NULL, NULL);

		//declare one mail struct
		mail fullmsg;
		//here we begin actually serving client (sender)
		while(true){
			bzero(str,100);
			read(comm,str,100);
			string input(str);
			cout<<input << endl;

			//state: if were in datamoode and read exit char, exit and contiue
			if(datamode && (input.compare(".\n")==0)){
				datamode = false;
				string temp("250 OK\n");
				write(comm, temp.c_str(), temp.size());
				continue;
			}
			//state: if in datamode w no escape continue reading 
			if(datamode){
				fullmsg.data+=input;
				continue;
			}

			string response = serverresponse(fullmsg, input)+"\n";
			//state: if we find 354 in response were entering datamode after writing
			if(response.find("354")!=string::npos){datamode = true;}
			bzero(str,100);
			write(comm,response.c_str(),response.size());
			if(response.find("221")==0){break;}
		}

		//write msg to file
		if(file.is_open()){
			file << "From:"<<fullmsg.from+"\n";
			file << "Date:"<< fullmsg.time+"\n";
			file << "To:" <<fullmsg.to.at(0)+"\n";
			file << fullmsg.data;
			file.close();
		}
		else{
			cout << "Unable to open file" << endl;
		}
		
		
	}







	else{
	//youre the sender. get commands from file then use hostname and connect to server
	
	//get file commands
		vector<string> all_lines;
		string line;
		try{
			ifstream file(myfile);
			bool maildone = false;
			bool rcptdone = false;
			bool smalldata = false;
			if(file.is_open()){
				while(getline(file,line)){
					line += "\n"; //getline messing up regex
					if(mailmatch(line) && !maildone){
						string whofrom(capturemail(line)[2]);
						all_lines.push_back("HELO "+whofrom+"\n");
						time_t mytime = time(0);
						all_lines.push_back(line.substr(0,line.size()-1) + " TIME:" + ctime(&mytime));
						maildone=true;
						continue;
					}
					if(rcptmatch(line)){rcptdone=true;}

					if(rcptdone && maildone && !(mailmatch(line)||rcptmatch(line))){
						all_lines.push_back("DATA\n");
						smalldata = true;
					}

					all_lines.push_back(line);
				}
				all_lines.push_back(".\n");
				all_lines.push_back("QUIT\n");
				//to see why i put this in this order or if you think i parsed inccorectly see the readme
				file.close();
			}
			else{
				cout << "File didn't open" << endl;
				return 1;
			}

		}
		catch(exception e){
			cout << "Error reading file" <<endl;
			return 1;
		}
	//done with file



	//connect to server to write
		int sockfd,n;
	    char sendline[100];
	    char recvline[100];
	    struct sockaddr_in servaddr;
	    struct hostent*server;
	 	
	    sockfd=socket(AF_INET,SOCK_STREAM,0);
	 
	 	//change for input name/addr
	    //inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));

	    server = gethostbyname(argv[2]);
	    if(server==NULL){
	    	cout << "Error, no such host" << endl;
	    	return 1;
	    }
	    else{
	    	bcopy((char*) &server->h_addr, (char*) &servaddr.sin_addr.s_addr, server->h_length);
	    }

	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family=AF_INET;
	    servaddr.sin_port=htons(portno);
	 
	    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0){
	    	cout << "Error connecting" << endl;
	    	return 1;
	    };


	 	//connected now act like sender
    	//memclears
        bzero( sendline, 100);
        bzero( recvline, 100);
        //fgets(sendline,100,stdin); //maybe change to cin to be consistent
 	
 		//send commands from file in order
 		for(int i=0; i<all_lines.size(); i++){
 			bzero( sendline, 100);
        	bzero( recvline, 100);
 			string tosend(all_lines.at(i));
 			cout << tosend;
 			this_thread::sleep_for(chrono::milliseconds(1000));
 			//if sending escape from datamode set to false to match server state
 			if(datamode && (tosend.compare(".\n")==0)){
 				datamode=false;			
 			}
			write(sockfd,tosend.c_str(),tosend.size());
			//if your sending data keep sending w/o expecting read (matches server state)
	 		if(datamode){continue;}

	       	read(sockfd,recvline,100);

	        //if servers ready for datamode we need to be too
	        string reply(recvline);	        	
	        if(reply.find("354")==0){datamode = true;}
        	cout <<reply << endl;
	   		if(reply.find("221")==0){break;}
	       
	       
	    	}
	}



}