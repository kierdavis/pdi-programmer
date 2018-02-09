import serial

class Timeout(Exception):
  pass

class PDIProgrammerError(Exception):
  pass

class PDIProgrammer(object):
  def __init__(self, ser):
    self.ser = ser

  def _send(self, byte):
    self.ser.write(chr(byte & 0xFF))
    ack = self.ser.read(1)
    if len(ack) == 0:
      raise Timeout

  def _recv(self):
    resp = self.ser.read(1)
    if len(resp) == 0:
      raise Timeout
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
  ser = serial.Serial("/dev/ttyUSB0", 57600, timeout=1)
  try:
    pdi = PDIProgrammer(ser)
    try:
      pdi.erase_chip()
      pdi.write_app_flash(0, [0x8f, 0xef, 0x80, 0x93, 0x01, 0x06, 0x80, 0x93, 0x07, 0x06, 0xfd, 0xcf])
      pdi.write_fuse(1, 0xff)
      pdi.write_fuse(2, 0xff)
      pdi.write_fuse(4, 0xff)
      pdi.write_fuse(5, 0xff)
    finally:
      pdi.close()
  finally:
    ser.close()

if __name__ == "__main__":
  main()