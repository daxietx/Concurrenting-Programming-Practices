#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

using namespace std;

string makefile = "";
bool autosearch = true;
bool pv = false;
bool kv = false;
bool dv = false;
bool iv = false;
bool tv = false;
int runtime;

map <string,string> macros;

struct Rule
{
	string target;                    //store targets
	vector<string>prereq;  //store prerequisites
	vector<string>cmd;      //store commands
}tmprule;

vector<struct Rule>rules;
vector<string> mypath;  // store  paths of searching directory
vector<string> checkedtar; // store the targets which are already checked

void signalhandler(int sig)
{
	kill(0,sig);
	exit(0);
}

void printmacros()
{
	map<string,string>::iterator it;
	for(it=macros.begin();it!=macros.end();it++)
	{
		cout<<it->first<<"="<<it->second<<endl;
	}
	cout<<endl;
}

void printrules()
{
	int i = 0;
	vector<struct Rule>::iterator it = rules.begin();
	for (;it!=rules.end();it++)
	{
		cout<<it->target<<" : ";
		vector<string>::iterator pit = it->prereq.begin();
		vector<string>::iterator cit =it->cmd.begin();
		for (;pit!=it->prereq.end();pit++)
		{
			cout<<*pit<<" ";
		}
		cout<<endl;
		for (;cit!=it->cmd.end();cit++)
		{
			cout<<"\t"<<*cit<<endl;
		}
		cout<<endl;
		i++;
	}
}

//check the macros in rules and replace them 
string check(string s)
{
	int i;
	unsigned int k;
	string tmp;
	while ((i=s.find('$'))>=0)
	{
		if (s[i+1]=='(')  //$ is followed by ()
		{
			k = s.find(')');
			tmp = macros.find(s.substr(i+2,k-i-2))->second;
			s.replace(i,k-i+1,tmp);
		}
		else    // $ is followed directly by the alias
		{
			k = i;
			while(k<=(s.size()-1))
			{
				if (s[k]!=' ')
					k++;
				else
					break;
			}		
			tmp = macros.find(s.substr(i+1,k-i-1))->second;
			s.replace(i,k-i,tmp);
		}
	}
	return s;
}

//remove the tab and space in the string
void trim(string &str)
{
	if (!str.empty())
	{
	  static std::string szEmptyCh=" \t\r\n";
	  std::string::size_type pos = str.find_last_not_of(szEmptyCh);
	  if(pos != std::string::npos) 
	  {
	   str.erase(pos + 1);
	   pos = str.find_first_not_of(szEmptyCh);
	   if(pos != std::string::npos) str.erase(0, pos);
	  }
	  else str.erase(str.begin(), str.end());
	}	
}

void parsefile(string file)
{
	int i;
	string buf="";
	string prebuf="";
	char* prebuf_c=NULL;
	char* tmppre=NULL;
	
	/*
	string str = file.substr(file.size()-3);
	if(strcmp(str.c_str(),".mk")!=0)
	{
		cerr<<"Wrong file type."<<endl;
		exit(1);
	}
	*/

	ifstream fp(file.c_str());

	if(!fp.is_open())
	{
		cerr<<"Fail to open file "<<file<<endl;
		exit(1);
	}
	// read the file first time to get all macros
	while(!fp.eof())
	{
		buf = "";
		getline(fp,buf);
		if((i=buf.find('#'))>=0)   //get comment
			buf = buf.substr(0,i); //remove comment
		if((i=buf.find('='))>0) //get macro
		{
			string alias = buf.substr(0,i);
			string value = buf.substr(i+1);
			trim(alias);
			trim(value);
			macros.insert(pair<string,string>(alias,value));
		}
	}
	fp.clear();
	fp.seekg(0,ios::beg);  // back to the begin of the file
	
	//read the file second time to get all targets, prerequisites and commands
	while(!fp.eof())
	{
		buf = "";
		getline(fp,buf);
		if((i=buf.find('#'))>=0)   //get comment
			buf = buf.substr(0,i); // remove comment
		if((i=buf.find('='))>0) //get macro, ignore
		{
			continue;
		}
		else if ((i=buf.find(':'))>0)  //get target and prerequisites
		{
			string target = buf.substr(0,i);
			trim(target);
			tmprule.target = target;   //get target
			//------------------deal with prerequisites-----------------------
			prebuf = buf.substr(i+1);
			prebuf_c = new char [prebuf.length()+1];
			strcpy(prebuf_c,prebuf.c_str());
			tmppre = strtok(prebuf_c," ");
			if (tmppre!=NULL)
			{
				string prereq(tmppre);
				trim(prereq);
				tmprule.prereq.push_back(prereq);
			}
			while(tmppre!=NULL)
			{
				tmppre = strtok(NULL," ");
				if (tmppre!=NULL)
				{
					string prereq(tmppre);
					trim(prereq);
					tmprule.prereq.push_back(prereq);
				}
			}
			delete[] prebuf_c;
			//------------------deal with prerequisites-----------------------		
			while(!fp.eof())
			{
				streampos pos = fp.tellg();
				getline(fp,buf);
				if ((i=buf.find('#'))>=0)
					buf = buf.substr(0,i);  //remove comment
				if ((i=buf.find(':'))>0)
				{
					fp.seekg(pos);    //go back one line
					break;
				}
				else if((i=buf.find('='))>0) //get macro
				{
					continue;
				}
				else if (buf.substr(0,1)=="\t" && buf.size()>1)  //get commands
				{
					buf = buf.substr(1);    //remove the tab
					string test = buf;
					trim(test);
					if(!test.empty())
						tmprule.cmd.push_back(buf);
				}
			}
			rules.push_back(tmprule);
			tmprule.target = "";
			tmprule.prereq.clear();
			tmprule.cmd.clear();		
		}
	}
}

void checkfile()
{
	vector<struct Rule>::iterator it;
	vector<string>::iterator pit;
	vector<string>::iterator cit;
	
	for(it=rules.begin();it!=rules.end();it++)
	{
		it->target = check(it->target);
		for(pit=it->prereq.begin();pit!=it->prereq.end();pit++)
			*pit = check(*pit);
		for(cit=it->cmd.begin();cit!=it->cmd.end();cit++)
			*cit = check(*cit);
	}

}

void countrules()
{
	int macronum = macros.size();
	int tarrulenum = 0;
	int infrulenum = 0;
	int cmdnum = 0;
	vector<struct Rule>::iterator it;
	string target;
	
	for(it=rules.begin();it!=rules.end();it++)
	{
		target = check(it->target);
		trim(target);
		if(target[0]=='.')
			infrulenum++;
		else
			tarrulenum++;		
		cmdnum = cmdnum + it->cmd.size();			
	}
	cout<<macronum<<" macros, "<<tarrulenum<<" target rules, "
	<<infrulenum<<" inf. rules, "<<cmdnum<<" cmds"<<endl;
}

int doexe(string cmd,string file="",bool RO=false,bool RI=false)
{
	string command = cmd;
	bool isbg = false;
	int j;
	if((j=cmd.find("&"))>=0)
	{
		cmd = cmd.substr(0,j);
		isbg = true;
	}
		
	stringstream ss;
	string str; //store each parameter
	ss<<cmd;
	int count = 0;
	while (ss>>str)
		count++;

	ss.str("");
	ss.clear();
	ss<<cmd;
	char** paralist = new char*[count+1];
	int i=0;
	while(ss>>str)                 //put all parameters in the parameter list
	{
		paralist[i] = new char[str.size()+1];
		strncpy(paralist[i],str.c_str(),str.size());
		paralist[i][str.size()] = 0;
		i++;
	}
	paralist[count] = NULL;
	
	if (strncmp(paralist[0],"echo",4)==0)  // if the command is an echo-command
	{
		for (int j=1;j<count;j++)
		{
			cout<<paralist[j]<<" ";
		}
		cout<<endl;
	}
	else
	{
		bool found = false;
		string prog;
		vector<string>::iterator pathit;
		for (pathit=mypath.begin();pathit!=mypath.end();pathit++)
		{
			prog = *pathit + "/" + paralist[0];  // the path with the executable name
			struct stat progbuf;
			if (stat(prog.c_str(),&progbuf)==0)
			{
				if (S_IXUSR & progbuf.st_mode) // check if the program is executable
				{
					found = true;
					break;
				}
			}		
		}

		if (found==false)
		{
			cerr<<"Fail to find executable "<<paralist[0]<<"."<<endl;
		}
		

		int pid = fork();
		if (pid==0)  // child process
		{
			if(RO)
			{
				close(STDOUT_FILENO);
				int fd;
				if((fd=open (file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777))<0)
				{
					cerr<<"Fail to open the redirect output file "<<file<<endl;
					//exit(0);
				}			
			}
			
			if(RI)
			{
				close(STDIN_FILENO);
				int fd;
				if((fd=open (file.c_str(), O_RDONLY , 0777))<0)
				{
					cerr<<"Fail to open the redirect input file "<<file<<endl;	
					//exit(0);
				}					
			}
			
			if (execv(prog.c_str(),paralist) == -1)			
			{
				cerr<<"Fail to execute the command \""<<cmd<<"\""<<endl;
			}				
			exit(0);	
		}
		else // parent process
		{
			int status;
			if(!isbg)  // work on foreground
				wait(&status);
		}
	}
	return 0;
}

void execmd(vector<struct Rule>::iterator it,int round) // the parameter should be the iterator of a rule vector
{
	if (it->cmd.size()>0)  //if there are some commands in the rule
	{
		vector<string>::iterator cit = it->cmd.begin();
		for (;cit!=it->cmd.end();cit++)
		{
			if(dv)
			{
				for(int m=0;m<round+1;m++)
					cout<<"    ";
				cout<<"Action: "<<*cit<<endl;
			}
			//--------------------------here lacks of execv code-----------------------
			string tmpstr = *cit;
			int i,j;
			if ((i=tmpstr.find("|"))>=0) //1:execute the commands concurrently in several pipes
			{			
				continue;   
			}
			else if ((i=tmpstr.find(";"))>=0)  //2:there are multiple commands in one line
			{
				stringstream ss;
				ss<<tmpstr;
				string str;
				bool dirchanged = false;
				char currdir[500];
				string tardir;
				getcwd(currdir,sizeof(currdir));  // log current directory
				while (getline(ss,str,';'))
				{
					if ((i=str.find("cd"))>=0)  // if the command is a cd command, then change the directory
					{
						stringstream ss2;
						ss2<<str;
						ss2>>tardir>>tardir;
						if (chdir(tardir.c_str())==-1)
						{
							cerr<<"The directory not found."<<endl;
						}						
						dirchanged  = true;
					}
					else
						doexe(str);
				}
				if (dirchanged)
				{
					chdir(currdir);  //if the directory changed, after the execution, change back to the original directory
				}
			}
			else if ((i=tmpstr.find(">"))>=0) // 3:a command with redirected Output
			{
				string prog = tmpstr.substr(0,i);
				string file = tmpstr.substr(i+1);
				trim(prog);
				trim(file);
				doexe(prog,file,true,false);					
			}
			else if ((j=tmpstr.find("<"))>=0) // 4:a command with redirected Input
			{
				string prog = tmpstr.substr(0,j);
				string file = tmpstr.substr(j+1);
				trim(prog);
				trim(file);
				doexe(prog,file,false,true);
			}
			else  //default:execute a simple command
			{
				doexe(tmpstr);
			}
		}
	}
}

int make(int round,string tomake="")
{	
	string tmppre,tmptar,tmpstr;
	string finaltar = tomake;
	int i;
	vector<struct Rule>::iterator it;
	vector<string>::iterator pit;
	vector<string>::iterator cit;
	bool istarget = false;
	bool needsupdate = false;
	struct stat tarbuf,prebuf;
	time_t tartime,pretime;

	if (tomake.empty())   //should do the complete make
	{
		it = rules.begin();
		finaltar = it->target;
	}	
	
	it = rules.begin();
	for (;it!=rules.end();it++)       // find whether the finaltar is a target
	{
		if ((*it).target.compare(finaltar) == 0)
		{
			istarget = true;
			break;
		}
	}
	
	if(istarget)
	{
		vector<string>::iterator checkit;
		for(checkit=checkedtar.begin();checkit!=checkedtar.end();checkit++)
		{
			if(finaltar.compare(*checkit)==0)
				return 0;
		}	
		checkedtar.push_back(finaltar);
	}
	
	if(dv)
	{
		if(istarget)
		{
			for(int m=0;m<round;m++)
				cout<<"    ";
			cout<<"Making "<<it->target<<endl;
			for(int m=0;m<round+1;m++)
				cout<<"    ";
			cout<<"Target rule for "<<it->target<<endl;
			if(!(it->prereq).empty())
			{
				for(int m=0;m<round+1;m++)
				cout<<"    ";
				cout<<"Prerequisites: ";
				vector<string>::iterator pit = it->prereq.begin();
				for(;pit!=it->prereq.end();pit++)
					cout<<*pit<<" ";
				cout<<endl;
			}
		}
		else
		{
			for(int m=0;m<round;m++)
				cout<<"    ";
			cout<<"Making "<<finaltar<<endl;			
		}
	}

	if (( i = stat(finaltar.c_str(),&tarbuf))==0 )  
	{
		if (istarget)  //case 1: the finaltar exists and is a target
		{
			
			tartime = tarbuf.st_mtime;			
			pit = it->prereq.begin();
			for (;pit!=it->prereq.end();pit++)
			{				
				make(round+2,*pit);
				if ((i=stat((*pit).c_str(),&prebuf))==0)
				{
					pretime = prebuf.st_mtime;
					if(tartime<pretime)
						needsupdate = true;
				}
			}
			if (needsupdate)
			{	
				execmd(it,round);
			}
			if(dv)
			{
				if(istarget)
				{
					for(int m=0;m<round;m++)
						cout<<"    ";
					cout<<"Done making "<<it->target<<endl;		
				}

			}		
			return 0;
		}
		else  //case 2: the finaltar exists and is not a target
		{
			if(dv)
			{
				for(int m=0;m<round;m++)
				cout<<"    ";
				cout<<"Done making "<<finaltar<<endl;		
			}
	
			return 0;   //it must be the source file and do nothing
		}
	}
	else // the finaltar  doesn't exists
	{
	    tarbuf.st_mtime = 0;
		if (istarget)  // case 3: the finaltar doesn't exists and it is a target
		{	
			needsupdate = true;
			tartime = tarbuf.st_mtime;
			pit = it->prereq.begin();
			for (;pit!=it->prereq.end();pit++)
			{				
				make(round+2,*pit);
				if ((i=stat((*pit).c_str(),&prebuf))==0)
				{
					pretime = prebuf.st_mtime;
					if(tartime<pretime)
						needsupdate = true;
				}
			}
			if (needsupdate)
			{				
				execmd(it,round);
			}
			if(dv)
			{
				if(istarget)
				{
					for(int m=0;m<round;m++)
						cout<<"    ";
					cout<<"Done making "<<it->target<<endl;		
				}
			}		
			return 0;
		}
		else // case 4: the finaltar doesn't exists and it isn't a target
		{
			cerr<<"The target "<<finaltar<<" doesn't exist."<<endl;
			return 1;  // there must be an error
		}
	}	
}

void searchfile()
{
	char currdir[500];
	DIR *dp;
	struct stat statbuf;
	struct dirent *content;
	bool found = false;
	getcwd(currdir,sizeof(currdir));
	
	if((dp=opendir(currdir))==NULL)
	{
		cerr<<"Fail to open the current directory."<<endl;
		exit(1);
	}
	while((content = readdir(dp))!=NULL)
	{
		char* name = content->d_name;
		for(int i=1;i<=3;i++)
		{
		
			string mymake("mymake");
			stringstream ss;
			string n;
			ss<<i;
			ss>>n;
			string tarfile = mymake+n+".mk";
			if(strcmp(name,tarfile.c_str())==0)
			{
				string str = string(currdir)+"/"+name;
				lstat(str.c_str(),&statbuf);
				if(S_ISREG(statbuf.st_mode))
				{
					found = true;
					makefile = string(name);
					break;
				}
			}
		}
		if(found)
			break;
	}
	closedir(dp);
	if(!found)
	{
		cerr<<"Fail to find makefile."<<endl;	
		exit(1);
	}
}


int main(int argc, char*argv[])
{
	string makepara="";
	int round = 0;
	
	//----------------handle path--------------------------
	char* path = getenv("MYPATH");
	char* tmppath = strtok(path,":");

	while(tmppath!=NULL)
	{
		if(strncmp(tmppath,".",1)==0)
		{
			char buf[500];
			getcwd(buf,sizeof(buf));
			mypath.push_back(string(buf));
		}
		else
			mypath.push_back(string(tmppath));
		tmppath = strtok(NULL,":");
	}	
	//-----------------------------------------------------------
	
	for(int i=1;i<argc;i++)
	{
		if(strncmp(argv[i],"-f",2)==0)
		{
			autosearch = false;
			makefile = string(argv[++i]); // get the name of makefile  
		}
		else if(strncmp(argv[i],"-p",2)==0)
		{
			pv = true;   // print commands
		}
		else if(strncmp(argv[i],"-k",2)==0)
		{
			kv = true;   //continue execution if some commands fail and print out error
		}
		else if(strncmp(argv[i],"-d",2)==0)
		{
			dv = true;  // print debugging info, including rules and commands
		}
		else if(strncmp(argv[i],"-i",2)==0)
		{
			iv = true;  // block the SIGINT and close the function of CTRL+C
		}
		else if(strncmp(argv[i],"-t",2)==0)
		{
			tv = true;
			runtime = atoi(argv[++i]);		
		}
		else
		{
			makepara = argv[i];
		}
	}
	
	if(tv)
		alarm(runtime);
		
	struct sigaction sa,alarm;
	sa.sa_handler = signalhandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	
	alarm.sa_handler = signalhandler;
	sigemptyset(&alarm.sa_mask);
	alarm.sa_flags = 0;
	
	sigset_t newmask, oldmask;
	sigemptyset(&newmask);
	sigaddset(&newmask,SIGINT);
	
	if(sigaction(SIGINT,&sa,NULL)==-1)
		cerr<<"Fail to handle SIGINT."<<endl;
	
	if(sigaction(SIGALRM,&alarm,NULL)==-1)
		cerr<<"Fail to handle ALARM."<<endl;
	

	if(iv==true)
	{
		if(sigprocmask(SIG_BLOCK,&newmask,&oldmask)<0)
			cerr<<"Fail to block SIGINT"<<endl;
	}
		
	if(autosearch)
	{
		searchfile(); // search for mymake1.mk ... 
	}
	
	parsefile(makefile);

	if(pv)
	{
		countrules();
		printmacros();
		printrules();	
		return 0;
	}
	
	checkfile();  //replace the alias in the rules
	
	//cout<<"now sleep.\n";
	//sleep(5);	
		
	if (make(round,makepara) == 1)
	{
		cerr<<"Make fails."<<endl;
	}
	
	return 0;
}