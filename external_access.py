#!/usr/bin/env python

import asyncore, asynchat
import os, socket, string, sys
import re
import signal
import threading

PIN_COUNT = 16
GPIO_HOST = "localhost"
GPIO_PORT = 4321

class AlarmException(Exception):
  def __init__(self):
    self.dont_update = 0

  def alarmHandler(self, signum, frame):
    self.dont_update = 1
    raise AlarmException

  def nonBlockingRawInput(self, prompt='', timeout=5):
    signal.signal(signal.SIGALRM, self.alarmHandler)
    signal.alarm(timeout)
    if self.dont_update == 1:
      temp_prompt = '\n'
    else:
      temp_prompt = prompt

    try:
      text = raw_input(temp_prompt)
      signal.alarm(0)
      return text
    except AlarmException:
      self.dont_update = 1
    except EOFError:
      return "ERROR"
    signal.signal(signal.SIGALRM, signal.SIG_IGN)
    return ''

class Server(asynchat.async_chat):
  def __init__(self, sock, server, pins_a, pins_b, pins_c, pins_d, pins_e, pins_f):
    asynchat.async_chat.__init__(self,sock)
    self.set_terminator("\r\n")
    self.header = None
    self.data = ""
    self.shutdown = 0
    self.notify = []
    self.server = server
    self.server.clients[sock.fileno()] = self

    self.gpiowmatch = re.compile(r"GPIO W . (\d{1,5}) (\d{1,5})")
    self.gpiormatch = re.compile(r"GPIO R . (\d{1,5})")
    self.pwmw_pan_match = re.compile(r"PWM W Pan ([-0+]?\d)")
    self.pwmw_tilt_match = re.compile(r"PWM W Tilt ([-0+]?\d)")

    self.pins_a = pins_a
    self.pins_b = pins_b
    self.pins_c = pins_c
    self.pins_d = pins_d
    self.pins_e = pins_e
    self.pins_f = pins_f

  def collect_incoming_data(self, data):
    self.data = self.data + data

  def found_terminator(self):
    if self.data.startswith("GPIO W "):
      m = self.gpiowmatch.match(self.data)
      if m:
        reg, val = m.groups()
        reg = int(reg)
        val = bin(int(val))
        print "Write :: Address: ", hex(reg), "Value: ", val
        if reg == 20:
          if self.data.startswith("GPIO W a "):
            for i in xrange(0, len(val) - 2):
              self.pins_a[-(i + 1)] = val[i + 2]
          elif self.data.startswith("GPIO W b "):
            for i in xrange(0, len(val) - 2):
              self.pins_b[-(i + 1)] = val[i + 2]
          elif self.data.startswith("GPIO W c "):
            for i in xrange(0, len(val) - 2):
              self.pins_c[-(i + 1)] = val[i + 2]
          elif self.data.startswith("GPIO W d "):
            for i in xrange(0, len(val) - 2):
              self.pins_d[-(i + 1)] = val[i + 2]
          elif self.data.startswith("GPIO W e "):
            for i in xrange(0, len(val) - 2):
              self.pins_e[-(i + 1)] = val[i + 2]
          elif self.data.startswith("GPIO W f "):
            for i in xrange(0, len(val) - 2):
              self.pins_f[-(i + 1)] = val[i + 2]
    elif self.data.startswith("GPIO R "):
      m = self.gpiormatch.match(self.data)
      if m:
        for s in self.data.split():
         if s.isdigit():
          reg_string = s
          reg = int(s)
        #print "Read  :: Address: ", reg
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
            # Unsupported GPIO device
            self.push('Unsupported')
    elif self.data.startswith("PWM W "):
      m = self.pwmw_pan_match.match(self.data)
      if m:
        val = m.groups()
        val = int(val[0])
        print "PWM Pan Angle is:", val, "degrees"

      m = self.pwmw_tilt_match.match(self.data)
      if m:
        val = m.groups()
        val = int(val[0])
        print "PWM Tilt Angle is:", val, "degrees"
    else:
      print "Received invalid command", repr(self.data)
    self.data = ""

  def handle_close(self):
    print "Closing the Handle"
    del self.server.clients[self.socket.fileno()]
    self.close()


class Connection(asyncore.dispatcher):
  def __init__(self):
    asyncore.dispatcher.__init__(self)
    self.port = GPIO_PORT
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) 
    self.bind(("", self.port))
    self.listen(1)
    self.clients = {}

    self.smatch = re.compile(r"GPIO S . (\d{1,5}) (\d{1,5})")

    self.pins_a = self.GetDefaultPins()
    self.pins_b = self.GetDefaultPins()
    self.pins_c = self.GetDefaultPins()
    self.pins_d = self.GetDefaultPins()
    self.pins_e = self.GetDefaultPins()
    self.pins_f = self.GetDefaultPins()

    self.num_clients = 0

  def handle_accept(self):
    print "Accepting Connection"
    self.num_clients = self.num_clients + 1
    conn, addr = self.accept()
    t = Server(conn, self, self.pins_a, self.pins_b, self.pins_c, self.pins_d, self.pins_e, self.pins_f)

  def read_exec_command(self, prompt):
    a = AlarmException()
    cmdline = a.nonBlockingRawInput(prompt=prompt)

    if cmdline == "ERROR":
      return 0

    self.execute_cmd(cmdline)

  def execute_cmd(self, cmd):
    if cmd.startswith("GPIO S "):
      m = self.smatch.match(cmd)
      if m:
        reg, val = m.groups()
        reg = int(reg)
        val = bin(int(val))
        print "Set   :: Address: ", hex(reg), "Value: ", val
        if int(reg) == 16:
          if cmd.startswith("GPIO S a "):
            for i in xrange(0, PIN_COUNT):
              self.pins_a[i] = 0

            for i in xrange(0, PIN_COUNT):
              if val[-i] == 'b':
                break
              self.pins_a[-i] = val[-i]

          elif cmd.startswith("GPIO S b "):
            for i in xrange(0, PIN_COUNT):
              self.pins_b[i] = 0

            for i in xrange(0, PIN_COUNT):
              if val[-i] == 'b':
                break
              self.pins_b[-i] = val[-i]

          elif cmd.startswith("GPIO S c "):
            for i in xrange(0, PIN_COUNT):
              self.pins_c[i] = 0

            for i in xrange(0, PIN_COUNT):
              if val[-i] == 'b':
                break
              self.pins_c[-i] = val[-i]

          elif cmd.startswith("GPIO S d "):
            for i in xrange(0, PIN_COUNT):
              self.pins_d[i] = 0

            for i in xrange(0, PIN_COUNT):
              if val[-i] == 'b':
                break
              self.pins_d[-i] = val[-i]

          elif cmd.startswith("GPIO S e "):
            for i in xrange(0, PIN_COUNT):
              self.pins_e[i] = 0

            for i in xrange(0, PIN_COUNT):
              if val[-i] == 'b':
                break
              self.pins_e[-i] = val[-i]

          elif cmd.startswith("GPIO S f "):
            for i in xrange(0, PIN_COUNT):
              self.pins_f[i] = 0

            for i in xrange(0, PIN_COUNT):
              if val[-i] == 'b':
                break
              self.pins_f[-i] = val[-i]

    elif cmd == '':
      return
    else:
      print "Received invalid command", repr(cmd)

  def handle_close(self):
    del self.server.clients[self.socket.fileno()]
    self.close()
    self.exit()

  def GetDefaultPins(self):
    print "Setting Pins"
    pins = []
    # Default all pins to 0
    for i in xrange(0, PIN_COUNT):
      pins.append('0')
    return pins


def main():
  s = Connection()

  print "Serving access to GPIO Panel"
  thread = threading.Thread(target=asyncore.loop,kwargs = {'timeout':1})
  thread.start()
  val = 1
  while val != 0:
    val = s.read_exec_command('(Netduio Plus 2) ')

  print "Exiting..."
  os.system("pkill python")

if __name__ == "__main__":

  main()
