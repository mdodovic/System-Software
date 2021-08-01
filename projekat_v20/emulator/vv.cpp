#include "terminal.h"
#include "cpu.h"
#include "emulatorexception.h"
#include "memory.h"

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>

struct termios stdin_backup_settings; // backup for standard input; in order to save settings before emulator execution

volatile int user_interrupt = 0; // status of console. User can press CTRL + C and stop the terminal
                                 // in that case; all settings needs to be returned to previous (using stdin_backup_settings)

void restoreFlags()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_backup_settings);
}

void cleanTerminal(int sig)
{
  restoreFlags();
  user_interrupt = 1;
}

void Terminal::setup()
{
  if (tcgetattr(STDIN_FILENO, &stdin_backup_settings) < 0)
  {
    // error!
    throw EmulatorException("Terminal cannot be started");
  }

  static struct termios raw = stdin_backup_settings;
  raw.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
  raw.c_cflag &= ~(CSIZE | PARENB);
  raw.c_cflag |= CS8;
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  if (atexit(restoreFlags) != 0)
  {
    throw EmulatorException("Cannot restore terminal");
  }

  atexit(restoreFlags);
  signal(SIGINT, cleanTerminal);

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw))
  {
    throw EmulatorException("Terminal cannot be configured");
  }
}

void Terminal::clean() { restoreFlags(); }

void Terminal::readInput()
{
  char c;
  if (read(STDIN_FILENO, &c, 1) == 1)
  {
    buffer.push(c);
  }

  if (buffer.size() > 0 && cpu.canRequest(CPU::TERMINAL_INTERRUPT))
  {
    memory.write(DATA_IN, buffer.front(), 1);
    buffer.pop();
    cpu.interruptMark(CPU::TERMINAL_INTERRUPT);
  }

  if (user_interrupt)
    exit(1);
}
