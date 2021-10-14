#include<unistd.h>
#include<stdlib.h>
#include<termios.h>
#include<ctype.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<sys/ioctl.h>
#include<bits/stdc++.h>
#include<dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include<iostream>
#include<unistd.h>
#include<bits/stdc++.h>
#include<vector>
#include <sys/wait.h>
using namespace std;
bool refresh = true;
vector<string> files;
stack<string> left_stack;
stack<string> right_stack;
char home[100];
enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
};
#define ABUF_INIT {NULL, 0}
int addPermZero=1;
// #define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))
/************DATA*******************/
struct editorConfig {
  int cx, cy;
  int screenrows;
  int screencols;
  int rowoff;
  struct termios orig_termios;
};
struct editorConfig E;
/*************Append buffer***********/
struct abuf
{
  /* data */
  char *b;
  int len;
};
void abAppend(struct abuf *ab, const char *s, int len) {
  char *n = (char *)realloc(ab->b, ab->len + len);
  if (n == NULL) return;
  memcpy(&n[ab->len], s, len);
  ab->b = n;
  ab->len += len;
}
void abFree(struct abuf *ab) {
  free(ab->b);
}


//***********OUTPUT*****************/
void editorScroll() {
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
}
std::vector<string> ls_l;
char* permissions(char *file){
    struct stat st;
    char *modeval = (char*)malloc(sizeof(char) * 9 + 1);
    if(stat(file, &st) == 0){
        mode_t perm = st.st_mode;
        modeval[0] = (perm & S_IRUSR) ? 'r' : '-';
        modeval[1] = (perm & S_IWUSR) ? 'w' : '-';
        modeval[2] = (perm & S_IXUSR) ? 'x' : '-';
        modeval[3] = (perm & S_IRGRP) ? 'r' : '-';
        modeval[4] = (perm & S_IWGRP) ? 'w' : '-';
        modeval[5] = (perm & S_IXGRP) ? 'x' : '-';
        modeval[6] = (perm & S_IROTH) ? 'r' : '-';
        modeval[7] = (perm & S_IWOTH) ? 'w' : '-';
        modeval[8] = (perm & S_IXOTH) ? 'x' : '-';
        modeval[9] = '\0';
        return modeval;     
    }
    else{
        return strerror(errno);
    }   
}

string getFileCreationTime(char *path) 
{
    // static int i=1;
    struct stat attr;
    stat(path, &attr);
    string ls;
    string perm = permissions(path);
    // cout<<perm[0]<<" ";
    if(addPermZero==1)
      ls = ls+perm[0];
    addPermZero=0;
    ls= ls + perm + "\t";
    // string y = to_string(*path);
    ls = ls + path + "\t";
    if(attr.st_uid=1000)
        ls+="yash\t";
    if(attr.st_gid=1000)
        ls+="yash\t";
    ls+=to_string(attr.st_size)+"B\t";

    string time = ctime(&attr.st_mtime); 
    for(int i=0; i<time.size()-1; i++)
      ls = ls+time[i];
    
    return ls;
}
void openDirectory(char *s, struct abuf *ab)
{
  DIR *d;
  // char cwd[100];
  struct dirent *dir;
  d = opendir(s);
  // getcwd(cwd, 100);
  chdir(s);
    if (d)
      {
          while ((dir = readdir(d)) != NULL)
          {
              // std::cout<< dir->d_name<<"\t";
              ls_l.push_back(getFileCreationTime(dir->d_name));
              files.push_back(dir->d_name);
          }
          closedir(d);
      }
    int y=0;
    abAppend(ab,"\033[H\033[J", 12);
    for(auto it:ls_l)
    {
      const char *s = it.c_str();
      abAppend(ab, s, it.size());
      // abAppend(ab, "\x1b[K", 3);
      y++;
      if (y < E.screenrows - 1) {
        abAppend(ab, "\r\n", 2);
      }
    }
}


void editorRefreshScreen() {
  editorScroll();
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  // char s[100];
  // getcwd(s,100);
  // editorDrawRows(s, &ab);
  // openDirectory(s, &ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);

}
void openDirUtil()
{
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  char s[100];
  getcwd(s,100);
  // editorDrawRows(s, &ab);
  openDirectory(s, &ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);

}

struct termios orig_termios;


// ***********TERMINAL*************/
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}
void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
int editorReadKey() {
  int nread;
  char c;
  // char seq[3];
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if(c=='e')
  {
    return c;
  }
  else if(c=='b')
  {
    return c;
  }
  else if(c=='h')
  {
    return c;
  }
  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    
    // if( seq[0] == '\\' && seq[1]=='n')
    //   cout<<"enter pressed";
    if (seq[0] == '[') {
      // cout<<"here";
      switch (seq[1]) {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
    }
    return '\x1b';
  }

  else 
    return c;
}
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}



//**************INPUT*****************/

void editorMoveCursor(int key) {
  switch (key) {
    case ARROW_LEFT:
      // E.cx--;
      // break;
      { 
      addPermZero=1;
      refresh=false;
      // string x = files[E.cy];
      if(left_stack.size()==0)
      {
          break;
      }
      string x = left_stack.top();
      right_stack.push(x);
      left_stack.pop();
      files.clear();
      ls_l.clear();
      char buffer[100];
      strcpy(buffer, x.c_str());
      struct abuf ab = ABUF_INIT;
      abAppend(&ab, "\x1b[?25l", 6);
      abAppend(&ab, "\x1b[H", 3);
      // editorDrawRows(s, &ab);
      openDirectory(buffer, &ab);
      char buf[32];
      snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
      abAppend(&ab, buf, strlen(buf));
      abAppend(&ab, "\x1b[?25h", 6);
      write(STDOUT_FILENO, ab.b, ab.len);
      abFree(&ab);
      break;
    }
      
    case ARROW_RIGHT:
      { 
      addPermZero=1;
      refresh=false;
      // string x = files[E.cy];
      if(right_stack.size()==0)
      {
          break;
      }
      string x = right_stack.top();
      // right_stack.push(x);
      right_stack.pop();
      files.clear();
      ls_l.clear();
      char buffer[100];
      strcpy(buffer, x.c_str());
      struct abuf ab = ABUF_INIT;
      abAppend(&ab, "\x1b[?25l", 6);
      abAppend(&ab, "\x1b[H", 3);
      // editorDrawRows(s, &ab);
      openDirectory(buffer, &ab);
      char buf[32];
      snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
      abAppend(&ab, buf, strlen(buf));
      abAppend(&ab, "\x1b[?25h", 6);
      write(STDOUT_FILENO, ab.b, ab.len);
      abFree(&ab);
      break;
    }
    case ARROW_UP:
    {
      E.cy--;
      refresh=false;
      break;
    }
    case ARROW_DOWN:
    {
      E.cy++;
      refresh=false;
      break;
    }
    case 'e':
    {
      addPermZero=1;
      refresh=false;
      string x = files[E.cy];
      // add execute file here
      char *y;
      // y = x.c_str();
      y = (char *)alloca(x.size() + 1);
      memcpy(y, x.c_str(), x.size()+1);
      string perm = permissions(y);
      if(perm[2] == '-')
      {
        string cmd = "code";
        char *args[3];
        args[0] = (char *)cmd.c_str();
        args[1] = (char *)x.c_str();
        args[2] = NULL;
        pid_t pid = fork();
        if(pid==0)
          execvp(args[0], args);
        if(pid > 0)
          wait(0);

        break;
      }
      files.clear();
      ls_l.clear();
      char cwd[100];
      getcwd(cwd, 100);
      left_stack.push(cwd);
      right_stack.push(cwd);
      char buffer[100];
      strcpy(buffer, x.c_str());
      struct abuf ab = ABUF_INIT;
      abAppend(&ab, "\x1b[?25l", 6);
      abAppend(&ab, "\x1b[H", 3);
      // editorDrawRows(s, &ab);
      openDirectory(buffer, &ab);
      char buf[32];
      snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
      abAppend(&ab, buf, strlen(buf));
      abAppend(&ab, "\x1b[?25h", 6);
      write(STDOUT_FILENO, ab.b, ab.len);
      abFree(&ab);
      break;
    }
    case 'b':
    {
      addPermZero=1;
      refresh=false;
      files.clear();
      ls_l.clear();
      char cwd[100];
      getcwd(cwd, 100);
      left_stack.push(cwd);
      right_stack.push(cwd);
      char buffer[100];
      string prev = "..";
      strcpy(buffer, prev.c_str());
      struct abuf ab = ABUF_INIT;
      abAppend(&ab, "\x1b[?25l", 6);
      abAppend(&ab, "\x1b[H", 3);
      // editorDrawRows(s, &ab);
      openDirectory(buffer, &ab);
      char buf[32];
      snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
      abAppend(&ab, buf, strlen(buf));
      abAppend(&ab, "\x1b[?25h", 6);
      write(STDOUT_FILENO, ab.b, ab.len);
      abFree(&ab);
      break;
    }
    case 'h':
    { 
      addPermZero=1;
      refresh=false;
      files.clear();
      ls_l.clear();
      char buffer[100];
      strcpy(buffer, home);
      struct abuf ab = ABUF_INIT;
      abAppend(&ab, "\x1b[?25l", 6);
      abAppend(&ab, "\x1b[H", 3);
      // editorDrawRows(s, &ab);
      openDirectory(buffer, &ab);
      char buf[32];
      snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
      abAppend(&ab, buf, strlen(buf));
      abAppend(&ab, "\x1b[?25h", 6);
      write(STDOUT_FILENO, ab.b, ab.len);
      abFree(&ab);
      break;
    }
  }
}
void editorProcessKeypress() {
  int c = editorReadKey();

  switch (c) {
    case 'q':
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    case ARROW_UP:
        editorMoveCursor(c);
        break;
    case ARROW_DOWN:
        editorMoveCursor(c);
        break;
    case ARROW_LEFT:
        editorMoveCursor(c);
        break;
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;
    case 'e':
      refresh=false;
      editorMoveCursor(c);
      break;
    case 'b':
      refresh = false;
      editorMoveCursor(c);
      break;
    case 'h':
      refresh = false;
      editorMoveCursor(c);
      break;
  }
}
/**********INIT**************/
void initEditor()
{
  E.cx = 0;
  E.cy = 0;
  E.rowoff=0;
  if(getWindowSize(&E.screenrows, &E.screencols)==-1) die("getWindowSize");
}
int main() {
  // cout << "\033[2J\033[1;1H";
  // system("clear");
  // std::cout << "" ;
  getcwd(home, 100);
  enableRawMode();
  initEditor();
  int x=0;
  while(1){
    if(!refresh)
      editorRefreshScreen();
    else
      openDirUtil();
    editorProcessKeypress();
  }
  
  // for(auto it:files)
    // cout<<it<<endl;
  return 0;
}