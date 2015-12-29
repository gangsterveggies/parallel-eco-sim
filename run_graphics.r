#! /usr/bin/env Rscript

library(ggplot2)
library(scales)

args <- commandArgs(trailingOnly = TRUE)
inp <- args[1]
out <- args[2]
lbl <- args[3]

dat <- read.csv(inp, header = TRUE, na.string = "", stringsAsFactors = TRUE)
times <- dat$tim

png(filename=paste(out, ".png", sep=""), bg="white", width=500, height=500)

ggplot(data=dat, aes(x=thr, y=tim, group=typ, color=typ)) +
    geom_line() +
    geom_point() +
    scale_x_continuous(trans = log2_trans(),
                       breaks = trans_breaks("log2", function(x) 2^x)) +
    scale_y_continuous(trans = log2_trans(),
                       breaks=times) +
    xlab("Número de threads") +
    ylab("Tempo de execução (segundos)") +
    ggtitle(paste("Resultados para", lbl)) +
    scale_colour_hue(name="Algoritmo", l=60) +
    scale_shape_manual(name="Algoritmo", values=c(26,25)) +
    scale_linetype_discrete(name="Algoritmo") +
    theme(legend.title = element_text(size = 12, face = "bold"),
          legend.text = element_text(size = 10, face = "bold"),
          plot.title = element_text(size=30, face="bold"))

dev.off()
