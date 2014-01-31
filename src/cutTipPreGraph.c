/*
 * cutTipPreGraph.c
 * 
 * Copyright (c) 2011-2013 BGI-Shenzhen <soap at genomics dot org dot cn>. 
 *
 * This file is part of SOAPdenovo-Trans.
 *
 * SOAPdenovo-Trans is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SOAPdenovo-Trans is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SOAPdenovo-Trans.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdinc.h"
#include "newhash.h"
#include "extfunc.h"
#include "extvab.h"

static int tip_c;
static int kmers_c;
double threshold; //0.05; mao 9-21
static long long *linearCounter;

static void Mark1in1outNode ();
static void thread_mark (KmerSet * set, unsigned char thrdID);

/*
static void printKmer(Kmer kmer)
{
    printKmerSeq(stdout,kmer);
    printf("\n");
}
*/
static int clipTipFromNode (kmer_t * node1, int cut_len, boolean THIN)
{
	unsigned char ret = 0, in_num, out_num, link;
	int sum, count;
	kmer_t *out_node;
	Kmer tempKmer, pre_word, word, bal_word;
	ubyte8 hash_ban;
	char ch1, ch;
	boolean smaller, found;
	int setPicker;
	unsigned int max_links, singleCvg;

	if (node1->linear || node1->deleted)
	{
		return ret;
	}

	if (THIN && !node1->single)
	{
		return ret;
	}

	in_num = count_branch2prev (node1);
	out_num = count_branch2next (node1);

	if (in_num == 0 && out_num == 1)
	{
		pre_word = node1->seq;

		for (ch1 = 0; ch1 < 4; ch1++)
		{
			link = get_kmer_right_cov (*node1, ch1);

			if (link)
			{
				break;
			}
		}

		word = nextKmer (pre_word, ch1);
	}
	else if (in_num == 1 && out_num == 0)
	{
		pre_word = reverseComplement (node1->seq, overlaplen);

		for (ch1 = 0; ch1 < 4; ch1++)
		{
			link = get_kmer_left_cov (*node1, ch1);

			if (link)
			{
				break;
			}
		}

		word = nextKmer (pre_word, int_comp (ch1));
	}
	else
	{
		return ret;
	}

	count = 1;
	bal_word = reverseComplement (word, overlaplen);

	if (KmerLarger (word, bal_word))
	{
		tempKmer = bal_word;
		bal_word = word;
		word = tempKmer;
		smaller = 0;
	}
	else
	{
		smaller = 1;
	}

	hash_ban = hash_kmer (word);
	setPicker = hash_ban % thrd_num;
	found = search_kmerset (KmerSets[setPicker], word, &out_node);

	if (!found)
	{
#ifdef MER127
		printf ("kmer %llx%llx%llx%llx not found, node1 %llx%llx%llx%llx\n", 
			word.high1, word.low1, word.high2, word.low2, 
			node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2);
#endif
#ifdef MER63
		printf ("kmer %llx%llx not found, node1 %llx%llx\n", 
			word.high, word.low, 
			node1->seq.high, node1->seq.low);
#endif
#ifdef MER31
		printf ("kmer %llx not found, node1 %llx\n", word,node1);
#endif
		exit (1);
	}

	while (out_node->linear)
	{
		count++;

		if (THIN && !out_node->single)
		{
			break;
		}

		if (count > cut_len)
		{
			return ret;
		}

		if (smaller)
		{
			pre_word = word;

			for (ch = 0; ch < 4; ch++)
			{
				link = get_kmer_right_cov (*out_node, ch);

				if (link)
				{
					break;
				}
			}

			word = nextKmer (pre_word, ch);
			bal_word = reverseComplement (word, overlaplen);

			if (KmerLarger (word, bal_word))
			{
				tempKmer = bal_word;
				bal_word = word;
				word = tempKmer;
				smaller = 0;
			}
			else
			{
				smaller = 1;
			}

			hash_ban = hash_kmer (word);
			setPicker = hash_ban % thrd_num;
			found = search_kmerset (KmerSets[setPicker], word, &out_node);

			if (!found)
			{
#ifdef MER127
				printf ("kmer %llx%llx%llx%llx not found, node1 %llx%llx%llx%llx\n", 
					word.high1, word.low1, word.high2, word.low2, 
					node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2);
#endif
#ifdef MER63
				printf ("kmer %llx%llx not found, node1 %llx%llx\n", 
					word.high, word.low, 
					node1->seq.high, node1->seq.low);
#endif
#ifdef MER31
				printf ("kmer %llx not found, node1 %llx\n", word,node1);
#endif
				printf ("pre_word %llx with %d(smaller)\n", pre_word, ch);
				exit (1);
			}
		}
		else
		{
			pre_word = bal_word;

			for (ch = 0; ch < 4; ch++)
			{
				link = get_kmer_left_cov (*out_node, ch);

				if (link)
				{
					break;
				}
			}

			word = nextKmer (pre_word, int_comp (ch));
			bal_word = reverseComplement (word, overlaplen);

			if (KmerLarger (word, bal_word))
			{
				tempKmer = bal_word;
				bal_word = word;
				word = tempKmer;
				smaller = 0;
			}
			else
			{
				smaller = 1;
			}

			hash_ban = hash_kmer (word);
			setPicker = hash_ban % thrd_num;
			found = search_kmerset (KmerSets[setPicker], word, &out_node);

			if (!found)
			{
#ifdef MER127
				printf ("kmer %llx%llx%llx%llx not found, node1 %llx%llx%llx%llx\n", 
					word.high1, word.low1, word.high2, word.low2, 
					node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2);
#endif
#ifdef MER63
				printf ("kmer %llx%llx not found, node1 %llx%llx\n", 
					word.high, word.low, 
					node1->seq.high, node1->seq.low);
#endif
#ifdef MER31
				printf ("kmer %llx not found, node1 %llx\n", word,node1);
#endif
				printf ("pre_word %llx with %d(larger)\n", reverseComplement (pre_word, overlaplen), int_comp (ch));//127kmer->31kmer
				exit (1);
			}
		}
	}

	if ((sum = count_branch2next (out_node) + count_branch2prev (out_node)) == 1)
	{
		tip_c++;
		node1->deleted = 1;
		out_node->deleted = 1;
		return 1;
	}
	else
	{
		ch = firstCharInKmer (pre_word);

		if (THIN)
		{
			tip_c++;
			node1->deleted = 1;
			dislink2prevUncertain (out_node, ch, smaller);
			out_node->linear = 0;
			return 1;
		}

		// make sure this tip doesn't provide most links to out_node
		max_links = 0;

		for (ch1 = 0; ch1 < 4; ch1++)
		{
			if (smaller)
			{
				singleCvg = get_kmer_left_cov (*out_node, ch1);

				if (singleCvg > max_links)
				{
					max_links = singleCvg;
				}
			}
			else
			{
				singleCvg = get_kmer_right_cov (*out_node, ch1);

				if (singleCvg > max_links)
				{
					max_links = singleCvg;
				}
			}
		}

		if (smaller && get_kmer_left_cov (*out_node, ch) < max_links)
		{
			tip_c++;
			node1->deleted = 1;
			dislink2prevUncertain (out_node, ch, smaller);

			if (count_branch2prev (out_node) == 1 && count_branch2next (out_node) == 1)
			{
				out_node->linear = 1;
			}

			return 1;
		}

		if (!smaller && get_kmer_right_cov (*out_node, int_comp (ch)) < max_links)
		{
			tip_c++;
			node1->deleted = 1;
			dislink2prevUncertain (out_node, ch, smaller);

			if (count_branch2prev (out_node) == 1 && count_branch2next (out_node) == 1)
			{
				out_node->linear = 1;
			}

			return 1;
		}
	}

	return 0;
}

void removeSingleTips ()
{
	int i, flag = 0, cut_len_tip;
	kmer_t *rs;
	KmerSet *set;

	//count_ends(hash_table);
	//cut_len_tip = 2*overlaplen >= maxReadLen4all-overlaplen+1 ? 2*overlaplen : maxReadLen4all-overlaplen+1;
	cut_len_tip = 2 * overlaplen;//mao 127kmer->31kmer
	printf ("Start to remove tips of single frequency kmers short than %d\n", cut_len_tip);
	tip_c = 0;

	for (i = 0; i < thrd_num; i++)
	{
		set = KmerSets[i];
		set->iter_ptr = 0;

		while (set->iter_ptr < set->size)
		{
			if (!is_kmer_entity_null (set->flags, set->iter_ptr))
			{
				rs = set->array + set->iter_ptr;
				flag += clipTipFromNode (rs, cut_len_tip, 1);
			}

			set->iter_ptr++;
		}
	}

	printf ("%d tips off\n", tip_c);
	Mark1in1outNode ();
}

void removeMinorTips ()
{
	int i, flag = 0, cut_len_tip;
	kmer_t *rs;
	KmerSet *set;

	//count_ends(hash_table);
	//cut_len_tip = 2*overlaplen >= maxReadLen4all-overlaplen+1 ? 2*overlaplen : maxReadLen4all-overlaplen+1;
	cut_len_tip = 2 * overlaplen;
	//if(cut_len_tip > 100) cut_len_tip = 100;
	printf ("Start to remove tips which don't contribute the most links\n");
	tip_c = 0;

	for (i = 0; i < thrd_num; i++)
	{
		set = KmerSets[i];
		flag = 1;

		while (flag)
		{
			flag = 0;
			set->iter_ptr = 0;

			while (set->iter_ptr < set->size)
			{
				if (!is_kmer_entity_null (set->flags, set->iter_ptr))
				{
					rs = set->array + set->iter_ptr;
					flag += clipTipFromNode (rs, cut_len_tip, 0);
				}

				set->iter_ptr++;
			}
		}

		printf ("kmer set %d done\n", i);
	}
	/*
	while (flag)
	{
		flag = 0;
		for (i = 0; i < thrd_num; i++)
		{
			set = KmerSets[i];
			set->iter_ptr = 0;

			while (set->iter_ptr < set->size)
			{
				if (!is_kmer_entity_null (set->flags, set->iter_ptr))
				{
					rs = set->array + set->iter_ptr;
					if (!rs->linear && !rs->deleted)
					{
						flag += clipTipFromNode (rs, cut_len_tip, 0);
					}
				}
				set->iter_ptr++;
			}
		}
//		printf ("kmer set %d done\n", i);
//		fprintf (stderr,"Remove minor tips in kmer set is done(round %d).\n", round++);
	}
*/
	printf ("%d tips off\n", tip_c);
	Mark1in1outNode ();
}

int getmaxofprev (kmer_t * node)
{
	int maxIn = 0, link, temp = 0;
	kmer_t *in_node;
	Kmer tempKmer, pre_word, word, bal_word;
	char ch1;
	boolean smaller, found;
	int setPicker;
	ubyte8 hash_ban;

	pre_word = node->seq;

	for (ch1 = 0; ch1 < 4; ch1++)
	{
		link = get_kmer_left_cov (*node, ch1);

		if (link)
		{
			word = prevKmer (pre_word, ch1);
			bal_word = reverseComplement (word, overlaplen);
			
			if (KmerLarger (word, bal_word))
			{
				tempKmer = bal_word;
				bal_word = word;
				word = tempKmer;
				smaller = 0;
			}
			else
			{
				smaller = 1;
			}

			hash_ban = hash_kmer (word);
			setPicker = hash_ban % thrd_num;
			found = search_kmerset (KmerSets[setPicker], word, &in_node);
			
			if (!found)
			{
#ifdef MER127
				printf ("source kmer:\t%llx%llx%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx%llx%llx\n",
					node->seq.high1, node->seq.low1, node->seq.high2,node->seq.low2,
					ch1,
					word.high1, word.low1, word.high2, word.low2
					);
#endif
#ifdef MER63
				printf ("source kmer:\t%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx\n",
					node->seq.high, node->seq.low, 
					ch1,
					word.high, word.low
					);
#endif
#ifdef MER31
				printf ("source kmer:\t%llu\tleft_ch:\t%d\tleft_kmer:\t%llu\n",node->seq,ch1,word);
#endif				
				exit (1);
			}
			/*
			if(smaller)
				temp = get_kmer_right_covs(*in_node);
			else
				temp = get_kmer_left_covs(*in_node);
			*/
			temp = in_node->count;
			
			if(temp > maxIn)
				maxIn = temp;
		}
	}
	
	return maxIn;
}

int getmaxofnext (kmer_t * node)
{
	int maxOut = 0, link, temp = 0;
	kmer_t *out_node;
	Kmer tempKmer, pre_word, word, bal_word;
	char ch1;
	boolean smaller, found;
	int setPicker;
	ubyte8 hash_ban;

	pre_word = node->seq;

	for (ch1 = 0; ch1 < 4; ch1++)
	{
		link = get_kmer_right_cov (*node, ch1);

		if (link)
		{
			word = nextKmer (pre_word, ch1);
			bal_word = reverseComplement (word, overlaplen);

			if (KmerLarger (word, bal_word))
			{
				tempKmer = bal_word;
				bal_word = word;
				word = tempKmer;
				smaller = 0;
			}
			else
			{
				smaller = 1;
			}
//			printf("link:\t%d\n",link);
//			printf("word:\t%llu\n",word);
//			printf("bal_word:\t%llu\n",bal_word);

			hash_ban = hash_kmer (word);
			setPicker = hash_ban % thrd_num;
			found = search_kmerset (KmerSets[setPicker], word, &out_node);
			
			if (!found)
			{
#ifdef MER127
				printf ("source kmer:\t%llx%llx%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx%llx%llx\n",
					node->seq.high1, node->seq.low1, node->seq.high2,node->seq.low2,
					ch1,
					word.high1, word.low1, word.high2, word.low2
					);
#endif
#ifdef MER63
				printf ("source kmer:\t%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx\n",
					node->seq.high, node->seq.low, 
					ch1,
					word.high, word.low
					);
#endif
#ifdef MER31
				printf ("source kmer:\t%llu\tleft_ch:\t%d\tleft_kmer:\t%llu\n",node->seq,ch1,word);
#endif	
				exit (1);
			}
			
			/*
			if(smaller)
				temp = get_kmer_left_covs(*out_node);
			else
				temp = get_kmer_right_covs(*out_node);
			*/
			temp = out_node->count;
			
			if(temp > maxOut)
				maxOut = temp;
		}
	}
	
	return maxOut;
}

static int clipKmerFromNode (kmer_t * node1)
{
	unsigned char ret = 0, in_num, out_num;
	ubyte4 maxIn = 0, maxOut = 0;

	int link, temp = 0;
	kmer_t *out_node, *in_node;
	Kmer tempKmer, pre_word, word, bal_word;
	char ch1, ch, tempch;
	boolean smaller, found;
	int setPicker;
	ubyte8 hash_ban;

	int link_out, temp_out = 0;
	kmer_t *out_node_out, *in_node_out;
	unsigned char in_num_out, out_num_out;
	Kmer tempKmer_out, pre_word_out, word_out, bal_word_out;
	boolean smaller_out, found_out;
	int setPicker_out;
	ubyte8 hash_ban_out;
	
	if (node1->linear || node1->deleted)// 
	{
		return ret;
	}
	
	in_num = count_branch2prev (node1);
	out_num = count_branch2next (node1);
	
	if(in_num <= 1 && out_num <= 1)
		return ret;
	
	if(in_num > 1)
	{
		maxIn = getmaxofprev(node1);
		if(maxIn)
		{
			pre_word = node1->seq;

			for (ch1 = 0; ch1 < 4; ch1++)
			{
				link = get_kmer_left_cov (*node1, ch1);

				if (link)
				{
					word = prevKmer (pre_word, ch1);
					bal_word = reverseComplement (word, overlaplen);
					
					if (KmerLarger (word, bal_word))
					{
						tempKmer = bal_word;
						bal_word = word;
						word = tempKmer;
						smaller = 0;
					}
					else
					{
						smaller = 1;
					}

					hash_ban = hash_kmer (word);
					setPicker = hash_ban % thrd_num;
					found = search_kmerset (KmerSets[setPicker], word, &in_node);
					
					if (!found)
					{
#ifdef MER127
						printf ("source kmer:\t%llx%llx%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx%llx%llx\n",
							node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2,
							ch1,
							word.high1, word.low1, word.high2, word.low2
							);
#endif
#ifdef MER63
						printf ("source kmer:\t%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx\n",
							node1->seq.high, node1->seq.low, 
							ch1,
							word.high, word.low
							);
#endif
#ifdef MER31
						printf ("source kmer:\t%llu\tleft ch:%d\tleft kmer:\t%llu\n",node1->seq, ch1,word);
#endif	 						
						exit (1);
					}

					/*
					if(smaller)
						temp = get_kmer_right_covs(*in_node);
					else
						temp = get_kmer_left_covs(*in_node);
					*/
					temp = in_node->count;
					
					if(temp && (double)temp / maxIn < threshold)
					{
						kmers_c++;
						in_node->deleted = 1;
						
						pre_word_out = in_node->seq;
						
						for (ch = 0; ch < 4; ch++)
						{
							link_out = get_kmer_left_cov (*in_node, ch);
							
							if (link_out)
							{
								word_out = prevKmer (pre_word_out, ch);
								bal_word_out = reverseComplement (word_out, overlaplen);
								
								if (KmerLarger (word_out, bal_word_out))
								{
									tempKmer_out = bal_word_out;
									bal_word_out = word_out;
									word_out = tempKmer_out;
									smaller_out = 0;
								}
								else
								{
									smaller_out = 1;
								}
								
								hash_ban_out = hash_kmer (word_out);
								setPicker_out = hash_ban_out % thrd_num;
								found_out = search_kmerset (KmerSets[setPicker_out], word_out, &in_node_out);
								
								if (!found_out)
								{
#ifdef MER127
									printf ("source kmer:\t%llx%llx%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx%llx%llx\n",
										node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2,
										ch1,
										word.high1, word.low1, word.high2, word.low2
										);
#endif
#ifdef MER63
									printf ("source kmer:\t%llx%llx\tleft_ch:\t%d\tleft_kmer:\t%llx%llx\n",
										node1->seq.high, node1->seq.low, 
										ch1,
										word.high, word.low
										);
#endif
#ifdef MER31
									printf ("source kmer:\t%llu\tleft ch:%d\tleft kmer:\t%llu\n",node1->seq, ch1,word);
#endif	 
									exit (1);
								}
								
								tempch = lastCharInKmer(pre_word_out);
								dislink2nextUncertain (in_node_out, tempch, smaller_out);
								
								if (count_branch2prev (in_node_out) == 1 && count_branch2next (in_node_out) == 1)
								{
									in_node_out->linear = 1;
								}
								else
								{
									in_node_out->linear = 0;
								}
							}
						}
						
						for (ch = 0; ch < 4; ch++)
						{
							link_out = get_kmer_right_cov (*in_node, ch);

							if (link_out)
							{
								word_out = nextKmer (pre_word_out, ch);
								bal_word_out = reverseComplement (word_out, overlaplen);
								
								if (KmerLarger (word_out, bal_word_out))
								{
									tempKmer_out = bal_word_out;
									bal_word_out = word_out;
									word_out = tempKmer_out;
									smaller_out = 0;
								}
								else
								{
									smaller_out = 1;
								}
								
								hash_ban_out = hash_kmer (word_out);
								setPicker_out = hash_ban_out % thrd_num;
								found_out = search_kmerset (KmerSets[setPicker_out], word_out, &out_node_out);
								
								if (!found_out)
								{
#ifdef MER127
									printf("source kmer:\t%llu %llu %llu %llu\n",
										node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2);
									printf("right_ch:%d\n",ch1);
									printf("right kmer:\t%llu %llu %llu %llu\n",
										out_node->seq.high1,out_node->seq.low1,out_node->seq.high2,out_node->seq.low2);
									printf("out_right_ch:\t%d\n",ch);
									printf("out_right_kmer:\t%llu %llu %llu %llu\n",
										word_out.high1,word_out.low1,word_out.high2,word_out.low2);
#endif
#ifdef MER63
									printf("source kmer:\t%llu %llu \n",
										node1->seq.high, node1->seq.low);
									printf("right_ch:%d\n",ch1);
									printf("right kmer:\t%llu %llu\n",
										out_node->seq.high,out_node->seq.low);
									printf("out_right_ch:\t%d\n",ch);
									printf("out_right_kmer:\t%llu %llu\n",
										word_out.high,word_out.low);							
#endif
#ifdef MER31
									printf ("source kmer:\t%llu\tright_ch:%d\tright kmer:\t%llu\tout_right_ch:\t%d\tout_right_kmer:\t%llu\n",									
										node1->seq, ch1,out_node->seq,ch,word_out);
#endif	 									
									exit (1);
								}

								tempch = firstCharInKmer(pre_word_out);
								dislink2prevUncertain (out_node_out, tempch, smaller_out);
								
								if (count_branch2prev (out_node_out) == 1 && count_branch2next (out_node_out) == 1)
								{
									out_node_out->linear = 1;
								}
								else
								{
									out_node_out->linear = 0;
								}
							}
						}
					}
				}
			}
		}
	}
	
	if(out_num> 1)
	{
		maxOut = getmaxofnext(node1);
		if(maxOut)
		{
			pre_word = node1->seq;

			for (ch1 = 0; ch1 < 4; ch1++)
			{
				link = get_kmer_right_cov (*node1, ch1);

				if (link)
				{
					word = nextKmer (pre_word, ch1);
					bal_word = reverseComplement (word, overlaplen);
					
					if (KmerLarger (word, bal_word))
					{
						tempKmer = bal_word;
						bal_word = word;
						word = tempKmer;
						smaller = 0;
					}
					else
					{
						smaller = 1;
					}

					hash_ban = hash_kmer (word);
					setPicker = hash_ban % thrd_num;
					found = search_kmerset (KmerSets[setPicker], word, &out_node);
					
					if (!found)
					{
#ifdef MER127
						printf ("kmer %llx%llx%llx%llx not found, node1 %llx%llx%llx%llx\n", 
							word.high1, word.low1, word.high2, word.low2, 
							node1->seq.high1, node1->seq.low1, node1->seq.high2,node1->seq.low2);
#endif
#ifdef MER63
						printf ("kmer %llx%llx not found, node1 %llx%llx\n", 
							word.high, word.low, 
							node1->seq.high, node1->seq.low);
#endif
#ifdef MER31
						printf ("source kmer:\t%llu\tright_ch:%d\tright_kmer:\t%llu\n",node1->seq, ch1,word);
#endif
						exit (1);
					}
					/*
					if(smaller)
						temp = get_kmer_left_covs(*out_node);
					else
						temp = get_kmer_right_covs(*out_node);
					*/
					temp = out_node->count;
					
					if(temp && (double)temp / maxOut < threshold)
					{
						kmers_c++;
						out_node->deleted = 1;

						pre_word_out = out_node->seq;
						for (ch = 0; ch < 4; ch++)
						{
							link_out = get_kmer_left_cov (*out_node, ch);

							if (link_out)
							{
								word_out = prevKmer (pre_word_out, ch);
								bal_word_out = reverseComplement (word_out, overlaplen);
								
								if (KmerLarger (word_out, bal_word_out))
								{
									tempKmer_out = bal_word_out;
									bal_word_out = word_out;
									word_out = tempKmer_out;
									smaller_out = 0;
								}
								else
								{
									smaller_out = 1;
								}
								
								hash_ban_out = hash_kmer (word_out);
								setPicker_out = hash_ban_out % thrd_num;
								found_out = search_kmerset (KmerSets[setPicker_out], word_out, &in_node_out);
								
								if (!found_out)
								{
#ifdef MER127
									printf ("kmer %llx%llx%llx%llx not found, out_node %llx%llx%llx%llx\n", 
										word_out.high1, word_out.low1, word_out.high2, word_out.low2, 
										out_node->seq.high1, out_node->seq.low1, out_node->seq.high2,	out_node->seq.low2);
#endif
#ifdef MER63
									printf ("kmer %llx%llx not found, out_node %llx%llx\n", 
										word_out.high, word_out.low, 
										out_node->seq.high, out_node->seq.low);
#endif
#ifdef MER31
									printf ("source kmer:\t%llu\tright_ch:%d\tright kmer:\t%llu\tin_right_ch:\t%d\tin_right_kmer:\t%llu\n",
										node1->seq, ch1,out_node->seq,ch,word_out);
#endif
									exit (1);
								}

								tempch = lastCharInKmer(pre_word_out);
								dislink2nextUncertain (in_node_out, tempch, smaller_out);
								
								if (count_branch2prev (in_node_out) == 1 && count_branch2next (in_node_out) == 1)
								{
									in_node_out->linear = 1;
								}
								else
								{
									in_node_out->linear = 0;
								}
							}
						}
						
						for (ch = 0; ch < 4; ch++)
						{
							link_out = get_kmer_right_cov (*out_node, ch);

							if (link_out)
							{
								word_out = nextKmer (pre_word_out, ch);
								bal_word_out = reverseComplement (word_out, overlaplen);
								
								if (KmerLarger (word_out, bal_word_out))
								{
									tempKmer_out = bal_word_out;
									bal_word_out = word_out;
									word_out = tempKmer_out;
									smaller_out = 0;
								}
								else
								{
									smaller_out = 1;
								}
								
								hash_ban_out = hash_kmer (word_out);
								setPicker_out = hash_ban_out % thrd_num;
								found_out = search_kmerset (KmerSets[setPicker_out], word_out, &out_node_out);
								
								if (!found_out)
								{
#ifdef MER127
									printf ("kmer %llx%llx%llx%llx not found, out_node %llx%llx%llx%llx\n", 
										word_out.high1, word_out.low1, word_out.high2, word_out.low2, 
										out_node->seq.high1, out_node->seq.low1, out_node->seq.high2,out_node->seq.low2);
#endif
#ifdef MER63
									printf ("kmer %llx%llx not found, out_node %llx%llx\n", 
										word_out.high, word_out.low, 
										out_node->seq.high, out_node->seq.low);
#endif

#ifdef MER31
									printf ("source kmer:\t%llu\tright_ch:%d\tright kmer:\t%llu\tout_right_ch:\t%d\tout_right_kmer:\t%llu\n",									
										node1->seq, ch1,out_node->seq,ch,word_out);
#endif									
									exit (1);
								}

								tempch = firstCharInKmer(pre_word_out);
								dislink2prevUncertain (out_node_out, tempch, smaller_out);
								
								if (count_branch2prev (out_node_out) == 1 && count_branch2next (out_node_out) == 1)
								{
									out_node_out->linear = 1;
								}
								else
								{
									out_node_out->linear = 0;
								}
							}
						}
					}
				}
			}
		}
	}
}

void removeMinorOut ()
{
	threshold= (double)dd/100;
	int i, flag = 0;
	kmer_t *rs;
	KmerSet *set;
	
	printf ("Start to remove kmer of out frequency kmers < %f\n",threshold);
	kmers_c = 0;
/*
	while (flag)
	{
		flag = 0;
		for (i = 0; i < thrd_num; i++)
		{
			set = KmerSets[i];
			set->iter_ptr = 0;

			while (set->iter_ptr < set->size)
			{
				if (!is_kmer_entity_null (set->flags, set->iter_ptr))
				{
					rs = set->array + set->iter_ptr;
//					if (!rs->linear && !rs->deleted)
					{
						flag += clipKmerFromNode (rs);
						fprintf (stderr,"%d\t%d\n", kmers_c,set->iter_ptr);
						printKmerSeq(stderr,rs->seq);
						fprintf(stderr,"\n");
					}
				}
				set->iter_ptr++;
			}
		}
//		fprintf (stderr,"Remove minor tips in kmer set is done(round %d).\n", round++);
	}*/
	
	for (i = 0; i < thrd_num; i++)
	{
		set = KmerSets[i];
		flag = 1;
		
//		while (flag)
		{
			flag = 0;
			set->iter_ptr = 0;
			
			while (set->iter_ptr < set->size)
			{
				if (!is_kmer_entity_null (set->flags, set->iter_ptr))
				{
					rs = set->array + set->iter_ptr;
					flag += clipKmerFromNode (rs);
//					fprintf (stderr,"%d\t%d\n", kmers_c,set->iter_ptr);
//					printKmerSeq(stderr,rs->seq);
//					fprintf(stderr,"\n");
				}
				set->iter_ptr++;
			}
		}
	}

	printf ("%d kmers off\n", kmers_c);
	Mark1in1outNode ();
}

static void threadRoutine (void *para)
{
	PARAMETER *prm;
	unsigned char id;

	prm = (PARAMETER *) para;
	id = prm->threadID;

	//printf("%dth thread with threadID %d, hash_table %p\n",id,prm.threadID,prm.hash_table);
	while (1)
	{
		if (*(prm->selfSignal) == 2)
		{
			*(prm->selfSignal) = 0;
			break;
		}
		else if (*(prm->selfSignal) == 1)
		{
			thread_mark (KmerSets[id], id);
			*(prm->selfSignal) = 0;
		}

		usleep (1);
	}
}

static void creatThrds (pthread_t * threads, PARAMETER * paras)
{
	unsigned char i;
	int temp;

	for (i = 0; i < thrd_num; i++)
	{
		if ((temp = pthread_create (&threads[i], NULL, (void *) threadRoutine, &(paras[i]))) != 0)
		{
			printf ("create threads failed\n");
			exit (1);
		}
	}

	printf ("%d thread created for cutTipPreGraph\n", thrd_num);
}

static void thread_mark (KmerSet * set, unsigned char thrdID)
{
	int in_num, out_num;
	kmer_t *rs;

	set->iter_ptr = 0;

	while (set->iter_ptr < set->size)
	{
		if (!is_kmer_entity_null (set->flags, set->iter_ptr))
		{
			rs = set->array + set->iter_ptr;

			if (rs->deleted || rs->linear)
			{
				set->iter_ptr++;
				continue;;
			}

			in_num = count_branch2prev (rs);
			out_num = count_branch2next (rs);

			if (in_num == 1 && out_num == 1)
			{
				rs->linear = 1;
				linearCounter[thrdID]++;
			}
		}

		set->iter_ptr++;
	}

	//printf("%lld more linear\n",linearCounter[thrdID]);
}

static void thread_wait (pthread_t * threads)
{
	int i;

	for (i = 0; i < thrd_num; i++)
		if (threads[i] != 0)
		{
			pthread_join (threads[i], NULL);
		}
}

static void sendWorkSignal (unsigned char SIG, unsigned char *thrdSignals)
{
	int t;

	for (t = 0; t < thrd_num; t++)
	{
		thrdSignals[t + 1] = SIG;
	}

	while (1)
	{
		usleep (10);

		for (t = 0; t < thrd_num; t++)
			if (thrdSignals[t + 1])
			{
				break;
			}

		if (t == thrd_num)
		{
			break;
		}
	}
}

static void Mark1in1outNode ()
{
	int i;
	long long counter = 0;
	pthread_t threads[thrd_num];
	unsigned char thrdSignal[thrd_num + 1];
	PARAMETER paras[thrd_num];

	for (i = 0; i < thrd_num; i++)
	{
		thrdSignal[i + 1] = 0;
		paras[i].threadID = i;
		paras[i].mainSignal = &thrdSignal[0];
		paras[i].selfSignal = &thrdSignal[i + 1];
	}

	creatThrds (threads, paras);
	thrdSignal[0] = 0;
	linearCounter = (long long *) ckalloc (thrd_num * sizeof (long long));

	for (i = 0; i < thrd_num; i++)
	{
		linearCounter[i] = 0;
	}

	sendWorkSignal (1, thrdSignal);	//mark linear nodes
	sendWorkSignal (2, thrdSignal);
	thread_wait (threads);

	for (i = 0; i < thrd_num; i++)
	{
		counter += linearCounter[i];
	}

	free ((void *) linearCounter);
	printf ("%lld linear nodes\n", counter);
}
