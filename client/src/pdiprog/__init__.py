import serial, sys

class PDIProgrammerError(Exception):
  pass

class PDIProgrammer(object):
  def __init__(self, ser):
    self.ser = ser

  def _send(self, byte):
    self.ser.write(chr(byte & 0xFF))
    ack = self.ser.read(1)
    if len(ack) == 0:
      raise PDIProgrammerError("timed out while waiting for acknowledgement")

  def _recv(self):
    resp = self.ser.read(1)
    if len(resp) == 0:
      raise PDIProgrammerError("timed out while waiting for acknowledgement")
    return ord(resp)

  def _check_response(self):
    resp = self._recv()
    if resp != 0:
      raise PDIProgrammerError(hex(resp))

  def erase_chip(self):
    self._send(0x01)
    self._check_response()

  def write_app_flash(self, addr, buf):
    self._send(0x02)
    self._send(addr & 0xFF)
    self._send((addr >> 8) & 0xFF)
    self._send((addr >> 16) & 0xFF)
    self._send((addr >> 24) & 0xFF)
    self._send(len(buf) & 0xFF)
    self._send((len(buf) >> 8) & 0xFF)
    for byte in buf:
      if isinstance(byte, str):
        byte = ord(byte)
      self._send(byte)
    self._check_response()

  def write_fuse(self, addr, data):
    self._send(0x03)
    self._send(addr)
    self._send(data)
    self._check_response()

  def close(self):
    self._send(0xFF)
    self._check_response()

def main():
  filename = sys.argv[1]
  with open(filename, "rb") as f:
    program = f.read()
  ser = serial.Serial("/dev/ttyUSB0", 57600, timeout=1)
  try:
    pdi = PDIProgrammer(ser)
    try:
      print "Erasing chip..."
      pdi.erase_chip()
      addr = 0
      total = len(program)
      while len(program):
        chunk = program[:1024]
        program = program[len(chunk):]
        perc = (addr * 100) / total
        print "Writing %d bytes at address %06Xh (%d%% complete)" % (len(chunk), addr, perc)
        pdi.write_app_flash(addr, chunk)
        addr += len(chunk)
      print "Writing fuses..."
      for fuse in [1, 2, 4, 5]:
        pdi.write_fuse(fuse, 0xff)
      print "Done."
    finally:
      try:
        pdi.close()
      except PDIProgrammerError:
        pass
  finally:
    ser.close()

if __name__ == "__main__":
  main()
