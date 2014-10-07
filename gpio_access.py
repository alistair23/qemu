#!/usr/bin/env python

import asyncore, asynchat
import os, socket, string, sys
import re
import signal

PIN_COUNT = 16
GPIO_HOST = "localhost"
GPIO_PORT = 4321

class AlarmException(Exception):
  def __init__(self):
    self.dont_update = 0

  def alarmHandler(self, signum, frame):
    self.dont_update = 1
    raise AlarmException

  def nonBlockingRawInput(self, prompt='', timeout=10):
    signal.signal(signal.SIGALRM, self.alarmHandler)
    signal.alarm(timeout)
    if self.dont_update == 1:
      temp_prompt = ''
    else:
      temp_prompt = prompt
    try:
        text = raw_input(temp_prompt)
        signal.alarm(0)
        return text
    except AlarmException:
      self.dont_update = 1
    signal.signal(signal.SIGALRM, signal.SIG_IGN)
    return ''

class Terminal(asynchat.async_chat):
  def __init__(self, sock, server):
    asynchat.async_chat.__init__(self,sock)
    self.set_terminator("\r\n")
    self.header = None
    self.data = ""
    self.shutdown = 0
    self.notify = []
    self.server = server
    self.server.clients[sock.fileno()] = self

    self.wmatch = re.compile(r"GPIO W . (\d{1,5}) (\d{1,5})")
    self.rmatch = re.compile(r"GPIO R . (\d{1,5})")

    self.pins_a = self.GetDefaultPins()
    self.pins_b = self.GetDefaultPins()
    self.pins_c = self.GetDefaultPins()
    self.pins_d = self.GetDefaultPins()
    self.pins_e = self.GetDefaultPins()
    self.pins_f = self.GetDefaultPins()

  def read_exec_command(self, prompt):
    a = AlarmException()
    cmdline = a.nonBlockingRawInput(prompt=prompt)

    self.execute_cmd(cmdline)

  def execute_cmd(self, cmd):
    if cmd.startswith("GPIO S "):
      print "Set   :: ", repr(cmd)
      print "Unimplemented - Coming Soon"
      # To do - Use this to set the value of the server regs
      # DO NOT push the command
    elif cmd == '':
      return
    else:
      print "Received invalid command", repr(cmd)

  def collect_incoming_data(self, data):
    self.data = self.data + data

  def found_terminator(self):
    if self.data.startswith("GPIO W "):
      m = self.wmatch.match(self.data)
      if m:
        reg, val = m.groups()
        reg = int(reg)
        print "Write :: Address: ", reg, "Value: ", val
        if reg == 20:
          if self.data.startswith("GPIO W a "):
            self.pins_a = val
          elif self.data.startswith("GPIO W b "):
            self.pins_b = val
          elif self.data.startswith("GPIO W c "):
            self.pins_c = val
          elif self.data.startswith("GPIO W d "):
            self.pins_d = val
          elif self.data.startswith("GPIO W e "):
            self.pins_e = val
          elif self.data.startswith("GPIO W f "):
            self.pins_f = val
    elif self.data.startswith("GPIO R "):
      m = self.rmatch.match(self.data)
      if m:
        for s in self.data.split():
         if s.isdigit():
          reg_string = s
          reg = int(s)
        print "Read  :: Address: ", reg
        if reg == 16:
          if self.data.startswith("GPIO R a "):
            self.push('GPIO R a '+''.join(map(str, self.pins_a)))
          elif self.data.startswith("GPIO R b "):
            self.push('GPIO R b '+''.join(map(str, self.pins_b)))
          elif self.data.startswith("GPIO R c "):
            self.push('GPIO R c '+''.join(map(str, self.pins_c)))
          elif self.data.startswith("GPIO R d "):
            self.push('GPIO R d '+''.join(map(str, self.pins_d)))
          elif self.data.startswith("GPIO R e "):
            self.push('GPIO R e '+''.join(map(str, self.pins_e)))
          elif self.data.startswith("GPIO R f "):
            self.push('GPIO R f '+''.join(map(str, self.pins_f)))
    else:
      print "Received invalid command", repr(self.data)
    self.data = ""

  def handle_close(self):
    print "Closing the Handle"
    del self.server.clients[self.socket.fileno()]
    self.close()

  def GetDefaultPins(self):
    pins = []
    # Default all pins to 0
    for i in xrange(0, PIN_COUNT):
      pins.append('1')
    return pins


class Server(asyncore.dispatcher):
  def __init__(self):
    asyncore.dispatcher.__init__(self)
    self.port = GPIO_PORT
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) 
    self.bind(("", self.port))
    self.listen(8)
    self.clients = {}

  def handle_accept(self):
    conn, addr = self.accept()
    t = Terminal(conn, self)
    t.read_exec_command('(Netduio Plus 2) ')


def main():
  s = Server()

  print "Serving access to GPIO Panel"
  while True:
    #if Terminal.read_exec_command(t, '(Netduio Plus 2) '):
    #  s.qemu_channel.push('0')
    asyncore.loop()

if __name__ == "__main__":

  main()
