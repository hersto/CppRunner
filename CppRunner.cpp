#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <vector>


/* To enable syntax coloring in VIM, do the following:

   In   /usr/share/vim/current/scripts.vim   there is a if-elseif section for various interpreter names
   found in the shebang line (first line starting with '#!'). Add this to the elseif section and you'll
   have C++ syntax highlighting for your cpprunner scripts.

  elseif s:name =~ 'cpprunner'
    set ft=cpp
    " Would actually have to examine the second line if it has a //#! to see which compiler is actually
    " called and in that case may set to another language.

*/



#define SizeOf(a)  (sizeof(a) / sizeof(*(a)))

#define C_Cast const_cast


class CFileCloser
{
public:
  CFileCloser(FILE* hFile): mhFile(hFile) {}
 ~CFileCloser() { if(mhFile) fclose(mhFile); }
  void DontClose() { mhFile= NULL; }
protected:
  FILE* mhFile;
};



std::string GetCurDir()
{
  int BuffLen= 1024;
  char* aBuff= (char*)malloc(BuffLen);
  if(!aBuff)
throw ENOMEM;

  while(true)
  {
    const char* CurWD= getcwd(aBuff, BuffLen);
    if(CurWD)
    {
      std::string Ret= CurWD;
      free(aBuff);
return Ret;
    }

    int NewBuffLen= BuffLen * 125 / 100;
    char* aNewBuff= (char*)realloc(aBuff, NewBuffLen);
    if(!aBuff)
    {
      free(aBuff);
throw ENOMEM;
    }
    aBuff= aNewBuff;
    BuffLen= NewBuffLen;
  }
}


typedef enum
{  eNo=  false
,  eYes= true
,  eDontKnow
}
  tTristate;


tTristate IsNewerThan(const char* FileA, const char* FileB)
{
  struct stat StatA;
  struct stat StatB;

  int rc= stat(FileA, &StatA);
  if(rc)
return eDontKnow;

  rc= stat(FileB, &StatB);
  if(rc)
return eDontKnow;

return StatA.st_mtime > StatB.st_mtime? eYes : eNo;
}



tTristate Exists(const char* File)
{
  struct stat Stat;
  int rc= stat(File, &Stat);
  if(!rc)
return eYes;

  if(errno == ENOENT)
return eNo;

  // In this case, we don't know if the file exists.
  // E.g. we might have received a permission denied for
  // a parent directory containing the file.
return eDontKnow;
}

// hFile must be a seekable stream, opened in binary mode.
std::string ReadLine(FILE* hFile, bool* pbEof, bool* pbErr)
{
  std::string Ret;
  char aBuffer[65536];

  while(true)
  {
    int NumReaded= fread(aBuffer, 1, SizeOf(aBuffer), hFile);

    for(int C= 0; C<NumReaded; C++ )
    {
      if(aBuffer[C] == '\n')
      {
        Ret.append(aBuffer, aBuffer+ C + 1);
        if(ferror(hFile))
return fprintf(stderr, "CppRunner: Could not read from file: %s\n", strerror(errno))
     , (*pbEof= false), (*pbErr= true), Ret;

        int rc= fseek(hFile, C - NumReaded + 1,  SEEK_CUR);
        if(rc != 0)
return fprintf(stderr, "CppRunner: Could not seek back to end of line after reading a bunch of bytes: %s\n", strerror(errno))
     , (*pbEof= false), (*pbErr= true), Ret;

return (*pbEof= feof(hFile)), (*pbErr= false), Ret;
      }
    }

    Ret.append(aBuffer, aBuffer+ NumReaded);

    if(feof(hFile))
return (*pbEof= feof(hFile)), (*pbErr= false), Ret;
  }
}

std::string QuoteMeta(const std::string& Arg)
{
  std::string Ret;
  for(int C= 0; C<Arg.length(); C++ )
  {
    char Ch= Arg[C];

    if(   Ch == '/' || Ch == '_' || Ch == '.' || Ch == '-'
       || Ch >= 'a' && Ch <= 'z'
       || Ch >= 'A' && Ch <= 'Z'
       || Ch >= '0' && Ch <= '9'
      )
      Ret+= Ch;
    else
      (Ret+= '\\'), (Ret+= Ch);
  }
return Ret;
}


bool ExtractSource(const std::string& Script, const std::string& Source)
{
  FILE* hScript= fopen(Script.c_str(), "rb");
  if(!hScript)
return fprintf(stderr, "CppRunner: Cannot open script \"%s\" for reading: %s\n", Script.c_str(), strerror(errno))
     , false;
  CFileCloser S_239844325(hScript);

  FILE* hSource= fopen(Source.c_str(), "wb");
  if(!hSource)
return fprintf(stderr, "CppRunner: Cannot open source file \"%s\" for writing: %s\n", Source.c_str(), strerror(errno))
     , false;
  CFileCloser S_Source(hSource);


  bool bFirstLine= true;
  while(true)
  {
    bool bEof, bErr;
    std::string Line= ReadLine(hScript, &bEof, &bErr);
    if(bErr)
return false;

    if(bFirstLine)
    {
      // If we have a hash-bang, we write only the line terminator. This is to not distort the line numbering in error messages.
      int NumCharsToWrite= strncmp(Line.c_str(), "#!", 2) == 0
                             ? (Line[Line.length() - 2] == '\r'? 2 : 1) // We know that the last char is a '\n'.
                             : Line.length();

      int rc= fprintf(hSource, "#line 1 \"%s\"%s", Script.c_str(), Line.c_str()+ Line.length() - (Line.length() >= 2? (Line[Line.length() - 2] == '\r'? 2 : 1) : 1));
      if(rc < 0)
return fprintf(stderr, "CppRunner: Cannot write to source file: %s\n", strerror(errno))
     , false;

      rc= fwrite(Line.c_str()+ Line.length() - NumCharsToWrite, 1, NumCharsToWrite, hSource);
      if(rc != NumCharsToWrite)
return fprintf(stderr, "CppRunner: Cannot write to source file: %s\n", strerror(errno))
     , false;

      if(bEof)
  break;

      bFirstLine= false;
    }else
    {
      int rc= fwrite(Line.c_str(), 1, Line.length(), hSource);

      if(rc != Line.length())
return fprintf(stderr, "CppRunner: Cannot write to source file: %s\n", strerror(errno))
     , false;

      if(bEof)
  break;
    }
  }

  S_Source.DontClose();
  int rc= fclose(hSource);
  if(rc != 0)
return fprintf(stderr, "CppRunner: Cannot close script file after writing to it: %s\n", strerror(errno))
     , false;

return true;
}


// Replaces the occurance of
//  {o} -> " -o "+ QuoteMeta(Exe) +" "
//  {i} -> " "+ QuoteMeta(Source) +" "
//  {}  -> " -o "+ QuoteMeta(Exe) +" "+ QuoteMeta(Source) +" "
// Provided they are not preceeded by a '\'.
// If the source and/or the exe has not been indicated by a marker, it is appended. In the case of the exe, prefixed with " -o ".
std::string AddSourceAndExeToCommand(const std::string& Cmd, const std::string& Source, const std::string& Exe)
{
  // Because our strings are quite short, and we have only one per execution,
  // we can do a simple approach of just appending char by char to the output variable.

  std::string Ret;

  typedef enum
  { eNormal= 0
  , eBackSlash
  , eBraceOpened
  , eBracedI
  , eBracedO
  }
    tState;

  tState State= eNormal;

  bool HaveSource= false;
  bool HaveExe=    false;

  for (int C= 0; C<Cmd.length();  C++ )
  {
    char Ch= Cmd[C];

    switch (State)
    {
    case eNormal:
        if (Ch == '\\')
          State= eBackSlash;
        else
        if (Ch == '{')
          State= eBraceOpened;
        else
          Ret+= Ch;
      break;

    case eBackSlash:
        Ret+= '\\';
        Ret+= Ch;
        State= eNormal;
      break;

    case eBraceOpened:
        if (Ch == 'i')
          State= eBracedI;
        else
        if (Ch == 'o')
          State= eBracedO;
        else
        if (Ch == '}')
        {
          Ret+= " -o "+ QuoteMeta(Exe) +" "+ QuoteMeta(Source) +" ";
          HaveSource=
          HaveExe=    true;
          State= eNormal;
        }else
throw "unsupported or incomplete  marker found in compiler hash bang."; // TBD: provide an adequate error message and bail out gracefully instead of throwing a string.
      break;

    case eBracedI:
        if (Ch == '}')
        {
          Ret+= " "+ QuoteMeta(Source) +" ";
          HaveSource= true;
          State= eNormal;
        }else
throw "unsupported or incomplete  marker found in compiler hash bang."; // TBD: provide an adequate error message and bail out gracefully instead of throwing a string.
      break;

    case eBracedO:
        if (Ch == '}')
        {
          Ret+= " -o "+ QuoteMeta(Exe) +" ";
          HaveExe= true;
          State= eNormal;
        }else
throw "unsupported or incomplete  marker found in compiler hash bang."; // TBD: provide an adequate error message and bail out gracefully instead of throwing a string.
      break;
    }
  }

  switch (State)
  {
  case eBackSlash:
throw "Concatenating compiler hashbang lines with a backslash not (yet) supported."; // TBD: provide an adequate error message and bail out gracefully instead of throwing a string.

  case eBraceOpened:
  case eBracedI:
  case eBracedO:
throw "unsupported or incomplete  marker found in compiler hash bang."; // TBD: provide an adequate error message and bail out gracefully instead of throwing a string.
  }

  if (!HaveExe)
    Ret+= " -o "+ QuoteMeta(Exe);

  if (!HaveSource)
    Ret+= " "+ QuoteMeta(Source);


return Ret;
}






bool CompileSource(const std::string& Source, const std::string& Exe)
{
  std::string Cmd;

  { FILE* hSource= fopen(Source.c_str(), "rb");
    if(!hSource)
return fprintf(stderr, "CppRunner: Cannot open source \"%s\" for reading the compiler options: %s\n", Source.c_str(), strerror(errno))
     , false;
    CFileCloser S_234234(hSource);

    std::string Line; // The line in question (the one with the compiler params) is line 3.

    bool bEof, bErr;
    ReadLine(hSource, &bEof, &bErr);
    if(bErr)
return false;

    if(!bEof)
    {
      ReadLine(hSource, &bEof, &bErr);
      if(bErr)
return false;

      if(!bEof)
      {
        Line= ReadLine(hSource, &bEof, &bErr);
        if(bErr)
return false;
      }
    }

    if(strncmp(Line.c_str(), "//#!", 4) == 0)   // TBD: We could also support multi-line comments beginning with /*#! and these
    {                                           //      then could be shell scripts - with /bin/sh being the default,
      int IdxStart= 4;                          //      but one could also specify the shell e.g. via /*#!/bin/bash .
      while(IdxStart < Line.length())           //      Or e.g. /*#!/usr/bin/perl -w . In order to also support things like #!environ(perl) -w
      {                                                        // i.e. that tool which figures out the path to a tool, i don't know its
        if(Line[IdxStart] == ' ' || Line[IdxStart] == '\t')    // name and syntax, we simply copy the code between /* and */ to a file,
          IdxStart++;                                          // do a   chmod u+x   on it and execute it. Source and Exe paths are passed
        else                                                   // in either as environment variables or as the two parameters, i don't know yet.
      break;
      }

      Cmd= Line.c_str() + IdxStart;
      if(Cmd.length() > 0 && Cmd[Cmd.length() - 1] == '\n')
        Cmd.resize(Cmd.length() -1);

      if(Cmd.length() > 0 && Cmd[Cmd.length() - 1] == '\r')
        Cmd.resize(Cmd.length() -1);
    }else
      Cmd= "g++";
  }


  Cmd= AddSourceAndExeToCommand(Cmd, Source, Exe);


  //if(we are verbose)
  //  printf("Executing: <%s>\n", Cmd.c_str());

  int rc= system(Cmd.c_str());

  if(rc < 0)
return fprintf(stderr, "CppRunner: Could not start compiler command: %s\n", strerror(errno))
     , false;

  if(rc != 0)
return fprintf(stderr, "CppRunner: Compilation failed: %s\n", strerror(errno))
     , false;

return true;
}




int main(int ArgC, char** ArgV)
{
  if(ArgC < 2)
return fprintf(stderr, "No script file given. CppRunner is a tool to compile and run C/C++ files via hashbang notation.\n")
     , 1;

  // First argument is the script file, additional arguments are arguments to be passed to the script.
  // Compiler options may be specified in the second line (not yet defined what format to used, and not yet implemented)

  try
  {
    std::string Script;
    if(ArgV[1][0] == '/')
    {
      Script= ArgV[1];
    }else
    {
      std::string CurDir= GetCurDir();

      Script= CurDir +"/"+ ArgV[1];
      // TBD: String things like /./ or /SomeDir/../ in the path.
    }

    std::string Base, Home;

    bool bIsInHomeS= false;
    { const char* aLastSlash= strrchr(C_Cast<char*>(Script.c_str()), '/');
      Base= aLastSlash + 1;

      const char* Home_asConstChar= getenv("HOME");
      if(!Home_asConstChar)
return fprintf(stderr, "CppRunner: Could not get value of $HOME variable.\n")
     , 1;

      Home= Home_asConstChar;

      std::string Prefix= Home +"/s/";

      if(Prefix.length() == aLastSlash - Script.c_str() + 1)
        if(0 == strncmp(Prefix.c_str(), Script.c_str(), Prefix.length())) // TBD: should we use strnicmp on CygWin and OS X?  On Cygwin this actually depends on the mount options.    Maybe it's better to compare dev:inode instead.
          bIsInHomeS= true;
    }

    if(!bIsInHomeS)
return fprintf(stderr, "CppRunner: Could not verify that the path \"%s\" points to the script in the ~/s/ directory.\n", Script.c_str())
     , 1;


    // It's important that the Source resides in the same directory as the script, so that #include statements still work.
    std::string Source= Home +"/s/.cpp-"+ Base +".cpp";
    std::string Exe=    Home +"/s/.cpp-"+ Base;

    tTristate ExistsScript= Exists(Script.c_str());
    if(ExistsScript == eDontKnow)
return fprintf(stderr, "CppRunner: Could not verify that the script file actually exists: %s\n", strerror(errno))
     , 1;

    tTristate ExistsSource= Exists(Source.c_str());
    if(ExistsSource == eDontKnow)
return fprintf(stderr, "CppRunner: Could not verify that the source file actually exists: %s\n", strerror(errno))
     , 1;

    tTristate ExistsExe= Exists(Exe.c_str());
    if(ExistsExe == eDontKnow)
return fprintf(stderr, "CppRunner: Could not verify that the executable file actually exists: %s\n", strerror(errno))
     , 1;

    if(!ExistsScript)
return fprintf(stderr, "CppRunner: The script file \"%s\" does not really exist.\n", Script.c_str())
     , 1;

    if(!ExistsSource || IsNewerThan(Script.c_str(), Source.c_str()) != eNo)
    {
      bool bOk= ExtractSource(Script, Source);
      if(!bOk)
return 1;
    }

    if(!ExistsExe || IsNewerThan(Source.c_str(), Exe.c_str()) != eNo)
    {
      bool bOk= CompileSource(Source, Exe); // Reads the g++ command line from the first line (something like   //#! g++ -lm -I /what/ever/path ), appends option  -o Exe.c_str()  and then calls this command.
      if(!bOk)
return 1;
    }

    std::vector<char*> AArgs;
    AArgs.resize(ArgC);
    for(int C= 0; C<ArgC - 1; C++ )
      AArgs[C]= ArgV[C + 1];
    AArgs[ArgC]= NULL;

    int rc= execv(Exe.c_str(), &AArgs.front());
    // If we return here, execv has failed.
    fprintf(stderr, "CppRunner: Could not execute the compiled program: %s\n", strerror(errno));
return 1;



  // Compare file times with ~/s/bin/$Base.cpp
  // If the script is newer, copy it to ~/s/bin/$Base.cpp omitting the first line with the hash-bang.
  // Compare file times of ~/s/bin/$Base.cpp with ~/s/bin/$Base
  // If $Base.cpp is newer than $Base, compile $Base.cpp to $Base
  // run $Base (with the parameters given)

  }catch(int Err)
  {
    fprintf(stderr, "Error %d: %s\n", (int)Err, strerror(Err));
return 1;
  }

return 0;
}
