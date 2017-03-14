k=$1
w=$2
mkdir k$k.w$w
../../../build/pandora index oxa_kmeans_k15_ordered_prg.fasta -k $k -w $w

for i in {2..532..2}
    do
	echo $i
	#head -n $i /data2/users/rachel/projects/reference_graphs/data/gram_neg_genefamilies/oxa/oxa.fasta | tail -n 2 &> oxa_read_$i.fasta
	../../../build/pandora map -p oxa_kmeans_k15_ordered_prg.fasta -r oxa_read_$i.fasta -o k$k.w$w/$i. -w $w -k $k -m 500 -e 0.0001 &> k$k/$i.log
	echo -e "$(head -1 oxa_read_$i.fasta)" &>> k$k.w$w/results.txt
	echo -e "$(head -2 oxa_read_$i.fasta | tail -n 1)\n$(head -2 k$k.w$w/$i._oxa_mlp.fasta | tail -n 1)" | ~/apps/cortex/scripts/analyse_variants/needleman_wunsch/needleman_wunsch --stdin --zam &>> k$k.w$w/results.txt

	echo -e "$(head -1 oxa_read_$i.fasta)" &>> k$k.w$w/new_results.txt
        echo -e "$(head -2 oxa_read_$i.fasta | tail -n 1)\n$(head -2 k$k.w$w/$i._oxa_kmlp.fasta | tail -n 1)" | ~/apps/cortex/scripts/analyse_variants/needleman_wunsch/needleman_wunsch --stdin --zam &>> k$k.w$w/new_results.txt

	echo -e "$(head -1 oxa_read_$i.fasta)" &>> k$k.w$w/diff_results.txt
        echo -e "$(head -2 k$k.w$w/$i._oxa_mlp.fasta | tail -n 1)\n$(head -2 k$k/$i._oxa_kmlp.fasta | tail -n 1)" | ~/apps/cortex/scripts/analyse_variants/needleman_wunsch/needleman_wunsch --stdin --zam &>> k$k.w$w/diff_results.txt
    done
#for i in {2..532..2}
#    do 
#        echo $i
#        ../../../build/pandora map -p oxa_kmeans_k15_ordered_prg.fasta -r oxa_read_$i.fasta -o k$k/$i. -w 1 -k $k -m 500 -e 0.0001 --output_p_dist &> k$k/$i.pdist.log
#    done
