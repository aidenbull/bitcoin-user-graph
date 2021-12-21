library(igraph)
library(ggplot2)

options(max.print=100)

edgeList = read.table("../output/userGraph-1-200000.txt", header=T)

userGraph = graph_from_data_frame(edgeList)

plot(userGraph)


degree_dist_out = degree_distribution(userGraph, mode="out", normalized=FALSE)

degree_dist_out_df = data.frame(degree_dist_out)

plot = ggplot(data=degree_dist_out_df, aes(x=as.numeric(rownames(degree_dist_out_df)), y=degree_dist_out)) + geom_line() + geom_point(shape=4) + xlab("degree") + ylab("probability") + scale_x_log10() + scale_y_log10() + annotation_logticks(sides="bl", scaled=TRUE)

ggsave("degree_dist_out_2.png", plot)



degree_dist_in = degree_distribution(userGraph, mode="in", normalized=FALSE)

degree_dist_in_df = data.frame(degree_dist_in)

plot = ggplot(data=degree_dist_in_df, aes(x=as.numeric(rownames(degree_dist_in_df)), y=degree_dist_in)) + geom_line() + geom_point(shape=4) + xlab("degree") + ylab("probability") + scale_x_log10() + scale_y_log10() + annotation_logticks(sides="bl", scaled=TRUE)

ggsave("degree_dist_in_2.png", plot)



transitivity(userGraph, type="global")

length(component_distribution(userGraph, mode="strong"))

#reciprocity(userGraph, mode="ratio")

#coreness(userGraph, mode="all")
