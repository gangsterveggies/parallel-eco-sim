import subprocess

threads = [1, 2, 4]
tests = [("input5x5", "output5x5"), ("input10x10", "output10x10"), ("input20x20", "output20x20")]

nok = False
for i in threads:
  for t in tests:
    runCommand = "./sim -np %d -pr" % i
    fin = open("inputs/%s" % t[0], 'rb')
    fout = open("tmp.tmp", 'wr')
    subprocess.call(runCommand.split(), stdin=fin, stdout=fout)

    cmpCommand = "diff inputs/%s tmp.tmp" % t[1]
    process = subprocess.Popen(cmpCommand.split(), stdout=subprocess.PIPE)
    output = process.communicate()[0]

    subprocess.call("rm tmp.tmp".split())

    if output != "":
      nok = True
      print "Error with %d in %s" % (i, t[0])

if not(nok):
  print "Everything went right!"
