import subprocess
import sys

#threads = [1, 2, 4, 8, 16, 32]
threads = [1, 2, 4, 8]

tests = [("input5x5", "S1"), ("input10x10", "S2"), ("input20x20", "M1"), ("input20x20", "L1")]
#tests = [("input100x100_unbal01", "S1"), ("input100x100_unbal02", "S2"), ("input200", "M1"), ("input500", "L1")]

algos = [("-al 0", "TP"), ("-al 1", "DD-0"), ("-al 2", "DD-1"), ("-al 3", "MX-0"), ("-al 4", "MX-1")]

for t in tests:
  print "Table for %s" % t[1]

  tbl = "typ,thr,tim\n"

  for extra in algos:
    for i in threads:
      runCommand = "./sim -np %d -tm %s" % (i, extra[0])

      fin = open("inputs/%s" % t[0], 'rb')
      process = subprocess.Popen(runCommand.split(), stdin=fin, stdout=subprocess.PIPE)
      output = process.communicate()[0]

      rtime = float(output[28:])

      tbl += "%s,%d,%f" % (extra[1], i, rtime)
      tbl += "\n"

  with open("results/%s.csv" % t[1], "w") as text_file:
    text_file.write(tbl)
  drawCommand = "./run_graphics.r results/%s.csv %s %s" % (t[1], t[1], t[1])
  subprocess.call(drawCommand.split())
