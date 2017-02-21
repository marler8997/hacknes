import std.stdio  : writeln, writefln, File;
import std.getopt : getopt;
import std.file   : exists;
import std.path   : setExtension;
import std.string : strip;
import std.conv   : to, ConvException;

void usage()
{
  writefln("nesc <source-file>");
}

class SilentException : Exception
{
  this()
  {
    super("Error already logged");
  }
}

enum _16K = 0x4000;

string sourceFilename;
File sourceFile;
uint lineNumber;
struct CodeBuilder
{
  // buffer.length will always be in increments of 16 K
  ubyte[] buffer;
  uint contentLength;
  this(bool ignoreMe)
  {
    buffer = new ubyte[_16K];
  }
  void ensureMoreCapacity(uint size)
  {
    if(contentLength + size > buffer.length) {
      throw new Exception("code larger than 16 K not implemented");
      buffer.length += _16K;
    }
  }
  void put(ubyte c)
  {
    ensureMoreCapacity(1);
    buffer[contentLength++] = c;
  }
  void put(ubyte c1, ubyte c2)
  {
    ensureMoreCapacity(2);
    buffer[contentLength++] = c1;
    buffer[contentLength++] = c2;
  }
  void put(ubyte c1, ubyte c2, ubyte c3)
  {
    ensureMoreCapacity(3);
    buffer[contentLength++] = c1;
    buffer[contentLength++] = c2;
    buffer[contentLength++] = c3;
  }
  void put(ubyte[] code)
  {
    ensureMoreCapacity(code.length);
    buffer[contentLength..contentLength+code.length] = code[];
  }
};
CodeBuilder codeBuilder = CodeBuilder(true);

inout(char)* skipWhitespace(inout(char)* str, inout(char)* limit)
{
  for(; str < limit && (*str == ' ' || *str == '\t'); str++) {
  }
  return str;
}
inout(char)* toWhitespace(inout(char)* str, inout(char)* limit)
{
  for(; str < limit && (*str != ' ' && *str != '\t'); str++) {
  }
  return str;
}
int main(string[] args)
{
  getopt(args);

  args = args[1..$];
  if(args.length == 0) {
    usage();
    return 0;
  }
  if(args.length > 1) {
    writefln("Error: too many command line arguments");
    return 1;
  }

  sourceFilename = args[0];
  if(!exists(sourceFilename)) {
    writefln("Error: source file '%s' does not exist", sourceFilename);
    return 1;
  }
  writefln("source file '%s'", sourceFilename);

  string outputFilename = sourceFilename.setExtension("nes");
  writefln("output file '%s'", outputFilename);
  lineNumber = 0;

  {
    char[] lineBuffer = new char[512]; // probably a good initial size
    sourceFile = File(sourceFilename, "r");
    while(sourceFile.readln(lineBuffer)) {
      lineNumber++;
      
      char* ptr = lineBuffer.ptr;
      char* limit = lineBuffer.ptr + lineBuffer.length;
      
      // strip ending newline
      // we do this now so we don't have to check for newline and
      // carriage return when we check for whitespace later.
      if(limit > ptr && *(limit-1) == '\n') {
        limit--;
        if(limit > ptr && *(limit-1) == '\r') {
          //writeln("[DEBUG] stripped \\r\\n");
          limit--;
        } else {
          //writeln("[DEBUG] stripped \\n");
        }
      }

      ptr = ptr.skipWhitespace(limit);
      if(ptr >= limit || *ptr == '#') {
        continue;
      }
      
      char[] firstWord;
      {
        char *start = ptr;
        ptr++;
        ptr = ptr.toWhitespace(limit);
        firstWord = start[0..ptr-start];
      }

      auto handler = firstWord in commands;
      if(handler is null) {
        writefln("%s(%s) Error: unknown command '%s'", sourceFilename, lineNumber, firstWord);
        return 1;
      }

      writefln("[DEBUG] Handling command '%s'", firstWord);
      ptr = ptr.skipWhitespace(limit); // might as well skip whitespace here
      (*handler)(ptr, limit);
    }
  }

  // render nes file
  return renderNesFile(File(outputFilename, "wb"));
}
immutable void function(char*,char*)[string] commands;
static this()
{
  commands["MirrorType"] = &MirrorTypeHandler;
  commands["BatteryBackedRam"] = &BatteryBackedRamHandler;
  commands["SEI"] = &SEIHandler;
  commands["CLD"] = &CLDHandler;
  // TODO: add commands
  // NmiLocation (sets the Nmi Handler vector table value to the current location)
  // IrqLocation (sets the Irq Handler vector table value to the current location)
  // ResetLocation (sets the reset location in the vector table)
}

// assumption: ptr is not at whitespace
char[] pullAndAssertOneArgument(string command, char* ptr, char* limit)
{
  if(ptr >= limit) {
    writefln("%s(%s) Error: command '%s' requires 1 argument but got 0",
             sourceFilename, lineNumber, command);
    throw new SilentException();
  }
  char* start = ptr;
  ptr++;
  ptr = ptr.toWhitespace(limit);
  if(ptr < limit) {
    writefln("%s(%s) Error: command '%s' requires 1 argument but got more",
             sourceFilename, lineNumber, command);
    throw new SilentException();
  }
  return start[0..ptr-start];
}
void assertNoArguments(string command, char* ptr, char* limit)
{
  if(ptr < limit) {
    writefln("%s(%s) Error: command '%s' requires 0 arguments but got more",
             sourceFilename, lineNumber, command);
    throw new SilentException();
  }
}

T parseCommandArgument(T)(const(char)[] str)
{
  try {
    return to!T(str);
  } catch(ConvException e) {
    writefln("%s(%s) Error: '%s' is not a valid %s",
             sourceFilename, lineNumber, str, T.stringof);
    throw new SilentException();
  }
}


enum MirrorType {
  Horizontal = 0x00,
  Vertical   = 0x01,
  UseVram    = 0x02,
}
struct RomSettings
{
  MirrorType mirrorType;
  bool batteryBackedRam;
}
RomSettings romSettings;

void MirrorTypeHandler(char* ptr, char* limit)
{
  romSettings.mirrorType = parseCommandArgument!MirrorType(pullAndAssertOneArgument("GraphicsType", ptr, limit));
}
void BatteryBackedRamHandler(char* ptr, char* limit)
{
  romSettings.batteryBackedRam = parseCommandArgument!bool(pullAndAssertOneArgument("BatteryBackedRam", ptr, limit));
}
void SEIHandler(char* ptr, char* limit)
{
  assertNoArguments("SEI", ptr, limit);
  codeBuilder.put(0x78);
}
void CLDHandler(char* ptr, char* limit)
{
  assertNoArguments("CLD", ptr, limit);
  codeBuilder.put(0xD8);
}


int renderNesFile(File outputFile)
{
  //
  // Render the NES header
  //
  outputFile.write("NES\x1A");
  
  writefln("codeBuilder.buffer.length = %s", codeBuilder.buffer.length);
  outputFile.write(cast(char)(codeBuilder.buffer.length / _16K));

  outputFile.write(cast(char)0); // [5] CHR rom is 0 for now
  // TODO: there is where the TrainerIsPresent flag would be set
  //       and also the lower nibble of the mapper number
  outputFile.write(cast(char)(
                   (romSettings.mirrorType & 0x01) |
                   ((romSettings.mirrorType & 0x02) << 2) |
                   (romSettings.batteryBackedRam ? 0x02 : 0x00)
                              )); // [6] 

  outputFile.write(cast(char)0); // [7] just use 0 for now
  outputFile.write(cast(char)0); // [8] size of PRG RAM in 8KB units
  outputFile.write(cast(char)0); // [9] just use 0 for now
  outputFile.write(cast(char)0); // [10] just use 0 for now
  outputFile.write("\0\0\0\0\0"); // [11-15] just use 0s for now

  //
  // TODO: insert trainer here if present
  //

  //
  // Insert PRG ROM
  //
  for(int i = 0; i < codeBuilder.contentLength; i++) {
    writefln("code[%s] = 0x%x", i, codeBuilder.buffer[i]);
  }
  outputFile.write(cast(char[])codeBuilder.buffer[0..codeBuilder.contentLength]);
  {
    auto extra = codeBuilder.contentLength % _16K;
    if(extra > 0) {
      auto padding = _16K - 6 - extra; // leave 6 bytes for interrupt vector table
      for(;padding > 16; padding -= 16) {
        outputFile.write("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
      }
      for(;padding > 0; padding--) {
        outputFile.write('\0');
      }
    }
    // The interrupt vector table
    outputFile.write("\x00\x80\x00\x80\x00\x80");
  }

  //
  // Insert CHR ROM
  //
  // TODO: insert code to add CHR ROM

  
  
  return 0;
}
