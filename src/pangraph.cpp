#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include "utils.h"
#include "pangraph.h"
#include "panread.h"
#include "panedge.h"
#include "pansample.h"
#include <cassert>
#include "minihit.h"
#include "tuplegraph.h"

#define assert_msg(x) !(std::cerr << "Assertion failed: " << x << std::endl)

using namespace std;

PanGraph::PanGraph() : next_id(0) {}

void PanGraph::clear() {
    for (const auto &c: reads) {
        delete c.second;
    }
    reads.clear();
    for (const auto &c: edges) {
        delete c;
    }
    edges.clear();
    for (const auto &c: nodes) {
        delete c.second;
    }
    nodes.clear();
    for (const auto &c: samples) {
        delete c.second;
    }
    samples.clear();
}

PanGraph::~PanGraph() {
    clear();
}

void PanGraph::add_node(const uint32_t prg_id, const string prg_name, const uint32_t read_id,
                        const set<MinimizerHitPtr, pComp> &cluster) {
    // check sensible things in new cluster
    for (const auto &it : cluster) {
        assert(read_id == it->read_id); // the hits should correspond to the read we are saying...
        assert(prg_id == it->prg_id);
    }

    // add new node if it doesn't exist
    PanNode *n;
    auto it = nodes.find(prg_id);
    if (it == nodes.end()) {
        n = new PanNode(prg_id, prg_id, prg_name);
        //cout << "add node " << *n << endl;
        nodes[prg_id] = n;
    } else {
        n = it->second;
        it->second->covg += 1;
        //cout << "node " << *n << " already existed " << endl;
    }

    // add a new read if it doesn't exist
    auto rit = reads.find(read_id);
    if (rit == reads.end()) {
        //cout << "new read " << read_id << endl;
        PanRead *r;
        r = new PanRead(read_id);
        r->add_hits(prg_id, cluster);
        reads[read_id] = r;
        n->reads.insert(r);
    } else {
        //cout << "read " << read_id  << " already existed " << endl;
        rit->second->add_hits(prg_id, cluster);
        n->reads.insert(rit->second);
    }

    assert(n->covg == n->reads.size());
}

void PanGraph::add_node(const uint32_t prg_id, const string &prg_name, const string &sample_name,
                        const vector<KmerNodePtr> &kmp, const LocalPRG *prg) {
    // add new node if it doesn't exist
    PanNode *n;
    auto it = nodes.find(prg_id);
    if (it == nodes.end()) {
        n = new PanNode(prg_id, prg_id, prg_name);
        n->kmer_prg = prg->kmer_prg;
        //cout << "add node " << *n << endl;
        nodes[prg_id] = n;
    } else {
        n = it->second;
        it->second->covg += 1;
        //cout << "node " << *n << " already existed " << endl;
    }

    // add a new sample if it doesn't exist
    auto sit = samples.find(sample_name);
    if (sit == samples.end()) {
        //cout << "new sample " << sample_name << endl;
        PanSample *s;
        s = new PanSample(sample_name);
        s->add_path(prg_id, kmp);
        samples[sample_name] = s;
        n->samples.insert(s);
        n->add_path(kmp);
    } else {
        //cout << "sample " << sample_name  << " already existed " << endl;
        sit->second->add_path(prg_id, kmp);
        n->samples.insert(sit->second);
        n->add_path(kmp);
    }

}

PanEdge *PanGraph::add_edge(const uint32_t &from, const uint32_t &to, const uint &orientation) {
    // NB this adds an edge from node_id from to node_id to
    //
    // checks
    auto from_it = nodes.find(from);
    auto to_it = nodes.find(to);
    assert((from_it != nodes.end()) and (to_it != nodes.end()));
    assert((orientation < (uint) 4) ||
           assert_msg("tried to add an edge with a rubbish orientation " << to_string(orientation) << " which should be < 4"));
    PanNode *f = (from_it->second);
    PanNode *t = (to_it->second);

    // update edges with new edge or increase edge coverage, and also add edge to read
    PanEdge *e;
    e = new PanEdge(f, t, orientation);
    pointer_values_equal<PanEdge> eq = {e};
    // if we have the edge, it will be added to from node, so just search the edges of that to see if it exists
    auto it = find_if(f->edges.begin(), f->edges.end(), eq);
    if (it == f->edges.end()) {
        //cout << "add edge " << *e << endl;
        edges.push_back(e);
        f->edges.push_back(e);
        t->edges.push_back(e);
        return e;
    } else {
        (*it)->covg += 1;
        //cout << "edge " << **it << " already in graph" << endl;
        delete e;
        return *it;
    }
}

void PanGraph::add_edge(const uint32_t &from, const uint32_t &to, const uint &orientation, const uint &read_id) {
    PanEdge *e;
    e = add_edge(from, to, orientation);

    PanRead *r;
    auto read_it = reads.find(read_id);
    if (read_it == reads.end()) {
        r = new PanRead(read_id);
        reads[read_id] = r;
    } else {
        r = (read_it->second);
    }

    r->edges.push_back(e);
    /*if (e->reads.find(r)!=e->reads.end())
    {
	cout << "have edge " << *e << " more than once in read " << *r << endl;
    }*/
    e->reads.insert(r);
    //cout << *e << " has read size " << e->reads.size() << endl;
    //cout << "added edge " << *e << endl;

    assert(e->covg == e->reads.size());
}

vector<PanEdge *>::iterator PanGraph::remove_edge(PanEdge *e) {
    //cout << "Remove graph edge " << *e << endl;
    // remove mentions of edge from nodes
    e->from->edges.erase(std::remove(e->from->edges.begin(), e->from->edges.end(), e), e->from->edges.end());
    e->to->edges.erase(std::remove(e->to->edges.begin(), e->to->edges.end(), e), e->to->edges.end());
    // remove mentions from reads. Note that if the edge is present in any reads, the vector of
    // edges along that read will become inconsistent
    for (auto r = e->reads.begin(); r != e->reads.end();) {
        //cout << "read was " << **r << endl;
        r = (*r)->remove_edge(e, r);
        //cout << "read is now " << **r << endl;
    }

    auto it = find(edges.begin(), edges.end(), e);
    delete e;
    it = edges.erase(it);
    //return iterator to next edge in pangraph
    return it;
}

map<uint32_t, PanNode *>::iterator PanGraph::remove_node(PanNode *n) {
    //cout << "Remove graph node " << *n << endl;
    // removes node n and for consistency all edges including n
    // this will remove those edges from reads which is likely to lead to
    // inconsistent vectors of edges in reads unless handled.
    for (auto i = n->edges.size(); i > 0; --i) {
        remove_edge(n->edges[i - 1]);
    }
    auto it = nodes.find(n->node_id);
    delete n;
    if (it != nodes.end()) {
        it = nodes.erase(it);
    }
    return it;
}

uint combine_orientations(uint f, uint t) {
    uint nice = f + t - 3 * (f > 1);
    uint fix = (f % 2 == 1) + 2 * (t > 1);
    if (nice != fix) {
        cout << "Warning, the edges were incompatible, but we handled it" << endl;
    }
    return fix;
}

vector<PanEdge *>::iterator PanGraph::add_shortcut_edge(vector<PanEdge *>::iterator prev, PanRead *r) {
    // creates a new edge to replace prev and prev+1 in read r
    PanEdge *e = nullptr;
    PanNode *n = nullptr;
    auto current = prev;
    current++;

    //have edges A->B and B->C, create edge A->C
    if ((*prev)->to->node_id == (*current)->from->node_id and (*prev)->from->node_id != (*current)->to->node_id) {
        e = add_edge((*prev)->from->node_id, (*current)->to->node_id,
                     combine_orientations((*prev)->orientation, (*current)->orientation));
        e->covg -= 1;
        n = (*prev)->to;
    } else if ((*prev)->to->node_id == (*current)->to->node_id and (*prev)->from->node_id != (*current)->from->node_id) {
        e = add_edge((*prev)->from->node_id, (*current)->from->node_id,
                     combine_orientations((*prev)->orientation, rev_orient((*current)->orientation)));
        e->covg -= 1;
        n = (*prev)->to;
    } else if ((*prev)->from->node_id == (*current)->to->node_id and (*prev)->to->node_id != (*current)->from->node_id) {
        e = add_edge((*prev)->to->node_id, (*current)->from->node_id,
                     combine_orientations(rev_orient((*prev)->orientation), rev_orient((*current)->orientation)));
        e->covg -= 1;
        n = (*prev)->from;
    } else if ((*prev)->from->node_id == (*current)->from->node_id and (*prev)->to->node_id != (*current)->to->node_id) {
        e = add_edge((*prev)->to->node_id, (*current)->to->node_id,
                     combine_orientations(rev_orient((*prev)->orientation), (*current)->orientation));
        e->covg -= 1;
        n = (*prev)->from;
    } else {
        // have a loop A->B and B->A, work out which node B is
        auto it = r->get_previous_edge(*prev);
        if (it == r->edges.end()) {
            it = r->get_next_edge(*current);
        }
        if (it != r->edges.end() and ((*it)->from == (*prev)->from or (*it)->to == (*prev)->from)) {
            // then to is the node to remove
            n = (*prev)->to;
        } else if (it != r->edges.end() and ((*it)->from == (*prev)->to or (*it)->to == (*prev)->to)) {
            n = (*prev)->from;
        }
    }

    // if n is not null, remove node from read r
    if (n != nullptr) {
        r->remove_node(n);
        assert(n->reads.size() == n->covg);
    }

    if (e != nullptr) {
        current = r->remove_edge(*current);
        prev = r->replace_edge(*prev, e);
        assert((*prev == e));
        assert(e->reads.size() == e->covg);
    } else {
        // this can only happen if we had something circular like A->B and B->A, in which case we want to delete both
        if (current + 1 != r->edges.end() and current + 2 != r->edges.end()) {
            current = r->remove_edge(*current);
            r->remove_edge(*prev);
            prev = current;
        } else {
            r->remove_edge(*current);
            r->remove_edge(*prev);
            prev = r->edges.end();
        }
    }

    return prev;
}

vector<PanEdge *>::iterator
PanGraph::split_node_by_edges(PanNode *n_original, PanEdge *e_original1, PanEdge *e_original2) {
    // results in two copies of n_original, one keeping the reads covered by e_original1
    // and the other without them.
    //cout << "Split node " << *n_original << " by edge " << *e_original1 << " (and " << *e_original2 << ")" << endl;
    assert(n_original == e_original1->from or n_original == e_original1->to);

    // give a unique id
    while (nodes.find(next_id) != nodes.end()) {
        next_id++;
    }

    // create new node
    PanNode *n;
    n = new PanNode(n_original->prg_id, next_id, n_original->name);
    n->covg = 0;
    nodes[next_id] = n;

    // create new edges
    PanEdge *e1, *e2;
    //cout << "add edge equivalent to " << *e_original1 << endl;
    if (n_original == e_original1->from) {
        //cout << "try to add edge " << n->node_id << " -> " << e_original1->to->node_id << " " << e_original1->orientation << endl;
        e1 = add_edge(n->node_id, e_original1->to->node_id, e_original1->orientation);
    } else if (n_original == e_original1->to) {
        //cout << "try to add edge " << e_original1->from->node_id << " -> " << n->node_id << " " << e_original1->orientation << endl;
        e1 = add_edge(e_original1->from->node_id, n->node_id, e_original1->orientation);
    } else {
        e1 = nullptr;
    }
    //cout << "add edge equivalent to " << *e_original2 << endl;
    if (n_original == e_original2->from) {
        //cout << "try to add edge " << n->node_id << " -> " << e_original2->to->node_id << " " << e_original2->orientation << endl;
        e2 = add_edge(n->node_id, e_original2->to->node_id, e_original2->orientation);
    } else if (n_original == e_original2->to) {
        //cout << "try to add edge " << e_original2->from->node_id << " -> " << n->node_id << " " << e_original2->orientation << endl;
        e2 = add_edge(e_original2->from->node_id, n->node_id, e_original2->orientation);
    } else {
        e2 = nullptr;
    }
    // we are going to add covg for each read, so subtract the covg added by creating the edge
    e1->covg -= 1;
    e2->covg -= 1;

    // amend reads
    //cout << "now fix reads" << endl;
    vector<PanEdge *>::iterator it;
    auto r = e_original1->reads.begin();
    while (r != e_original1->reads.end()) {
        // add read to new node n and remove from n_original
        (*r)->replace_node(n_original, n);

        // replace e_original2 in read
        (*r)->replace_edge(e_original2, e2);

        // replace e_original1 in read
        r = (*r)->replace_edge(e_original1, e1, r);

    }
    // be greedy and also steal edges along e_original2 if it is the only edge in the read
    r = e_original2->reads.begin();
    while (r != e_original2->reads.end()) {
        if ((*r)->edges.size() == 1) {
            // add read to new node n and remove from n_original
            (*r)->replace_node(n_original, n);

            // replace e_original2 in read
            r = (*r)->replace_edge(e_original2, e2, r);
        } else {
            ++r;
        }
    }

    //cout << "delete edges if they have 0 covg" << endl;
    // if e_original1 or e_original2 now have 0 covg, delete both from nodes and graph
    if (e_original2->covg == 0) {
        assert(e_original2->reads.empty() ||
               assert_msg("e_original2->reads.size() == " << e_original2->reads.size()));
        remove_edge(e_original2);
    }

    //cout << "same for original edge" << endl;
    assert(e_original1->covg == 0 || assert_msg("e_original1->covg == " << e_original1->covg));
    assert(e_original1->reads.empty() || assert_msg("e_original1->reads.size() == " << to_string(e_original1->reads.size())));
    it = find(n_original->edges.begin(), n_original->edges.end(), e_original1);
    it = n_original->edges.erase(it);
    remove_edge(e_original1);

    return it;
}

void PanGraph::split_nodes_by_reads(const uint &node_thresh, const uint &edge_thresh) {
    cout << now() << "Start breaking up pannodes by reads" << endl;
    for (auto n : nodes) {
        if (n.second->edges.size() > 2 and n.second->covg > node_thresh) {
            auto it = n.second->edges.begin();
            while (it != n.second->edges.end() and n.second->edges.size() > 2) {
                if ((*it)->covg <= edge_thresh) {
                    ++it;
                    continue;
                }
                // see if all reads along this edge enter/leave the node along the same edge
                unordered_set<PanEdge *> s;
                for (auto r : (*it)->reads) {
                    //cout << "look at node " << n.first << " edge " << **it << " and read " << *r << endl;
                    auto found = r->get_other_edge(*it, n.second);
                    if (found != r->edges.end()) {
                        //cout << "found edge " << **found << endl;
                        s.insert(*found);
                    }
                    if (s.size() > 1) {
                        break;
                        /*} else {
                cout << "set contains: ";
                for (auto i : s)
                {
                    cout << *i << endl;
                }*/
                    }
                }

                // if they do, clone the node
                if (s.size() == 1) {
                    it = split_node_by_edges(n.second, *it, *s.begin());
                } else {
                    ++it;
                }
            }
        }
    }
    cout << now() << "Finished splitting nodes by reads. PanGraph now has  " << nodes.size() << " nodes and " << edges.size()
         << " edges" << endl;
}

void PanGraph::read_clean(const uint &thresh) {
    cout << now() << "Start read cleaning with threshold " << thresh << " and " << edges.size() << " edges" << endl;

    for (auto &read : reads) {
        if (read.second->edges.size() < 2) {
            continue;
        }
        vector<PanEdge *>::iterator current;
        for (auto prev = read.second->edges.begin();
             prev != read.second->edges.end() and prev != --read.second->edges.end();) {
            current = prev;
            current++;
            //cout << "consider edges" << **prev << " and " << **current << endl;
            if ((*prev)->covg <= thresh and (*current)->covg <= thresh) {
                //cout << "read " << read.first << " edges " << **prev << " and " << **current << " have low covg "
                //     << endl;
                prev = add_shortcut_edge(prev, read.second);
                //cout << "read is now: " << *(read.second) << endl;
            } else {
                prev = current;
            }
        }
    }
    cout << now() << "Finished read cleaning. PanGraph now has " << edges.size() << " edges" << endl;
}

void PanGraph::remove_low_covg_nodes(const uint &thresh) {
    cout << now() << "Remove nodes with covg <= " << thresh << endl;
    for (auto it = nodes.begin(); it != nodes.end();) {
        //cout << "look at node " << *(it->second) << endl;
        if (it->second->covg <= thresh or (!edges.empty() and it->second->edges.empty())) {
            //cout << "delete node " << it->second->name;
            it = remove_node(it->second);
            //cout << " so pangraph now has " << nodes.size() << " nodes" << endl;
        } else {
            ++it;
        }
    }
    cout << now() << "Pangraph now has " << nodes.size() << " nodes" << endl;
}

void PanGraph::remove_low_covg_edges(const uint &thresh) {
    cout << now() << "Remove edges with covg <= " << thresh << endl;
    for (auto it = edges.begin(); it != edges.end();) {
        //cout << "look at edge " << **it << endl;
        if ((*it)->covg <= thresh) {
            //cout << "delete edge " << **it << endl;
            it = remove_edge(*it);
        } else {
            ++it;
        }
    }
    cout << now() << "Pangraph now has " << edges.size() << " edges" << endl;
}

/*reroute_edge(PanEdge* e, PanEdge* d1, PanEdge* d2)
{
    if (combine_orientations((*out)->orientation, (*to)->orientation) == it->orientation)
    {
    }

void PanGraph::remove_triangles(const uint& thresh)
{
    // removes triangles where A-lo->C and A-hi->B-hi->C
    cout << now() << "remove triangles using threshhold " << thresh << endl;
    for(vector<PanEdge*>::iterator it=edges.begin(); it!=edges.end();)
    {
        //cout << "look at edge " << **it << endl;
        if ((*it)->covg <= thresh)
        {
            //cout << "delete edge " << **it << endl;
	    for(vector<PanEdge*>::iterator out=from->edges.begin(); out!=from->edges.end();)
	    {
		for(vector<PanEdge*>::iterator in=to->edges.begin(); in!=to->edges.end();)
            	{
		    if ((*out)->covg > 2*thresh and (*in)->covg > 2*thresh)
		    {
			if ((*out)->from == (*in)->from)
			{
			    for 
			} else if ((*out)->from == (*in)->to)
                        {
                        } else if ((*out)->to == (*in)->from)
			    reroute_reads(*it, *out, *in)(combine_orientations((*out)->orientation, (*to)->orientation) == it->orientation or )
                        {
			    for (auto r: reads)
			    {
				
			    }
                        } else if ((*out)->to == (*in)->to)
                        {
                        }
		    }
            (*it)->from->edges.erase(std::remove((*it)->from->edges.begin(), (*it)->from->edges.end(), (*it)), (*it)->from->edges.end());
            (*it)->to->edges.erase(std::remove((*it)->to->edges.begin(), (*it)->to->edges.end(), (*it)), (*it)->to->edges.end());
            delete *it;
            it = edges.erase(it);
        } else {
            ++it;
        }
    }
}*/

void PanGraph::clean(const uint32_t &coverage) {
    // get total edge covg / total node covg (will be < 1 is short reads and closer to 1 if long reads)
    uint edge_covg = 0, node_covg = 0;
    for (uint i = 0; i != edges.size(); ++i) {
        edge_covg += edges[i]->covg;
    }
    cout << "Edge covg " << edge_covg << endl;
    for (auto n : nodes) {
        node_covg += n.second->covg;
    }
    cout << "Node covg " << node_covg << endl;
    float edges_per_node = coverage * edge_covg / node_covg;
    cout << "Factor " << edges_per_node << endl;
    read_clean(0.025 * edges_per_node);
    read_clean(0.05 * edges_per_node);
    read_clean(0.1 * edges_per_node);
    read_clean(0.2 * edges_per_node);
    //remove_low_covg_edges(0);
    //remove_low_covg_nodes(0);
    //remove_low_covg_nodes(0.05*coverage);

    split_nodes_by_reads(1.5 * coverage, edges_per_node);
    read_clean(0.2 * edges_per_node);

    remove_low_covg_edges(0.2 * edges_per_node);
    remove_low_covg_nodes(0.05 * coverage);
}

void PanGraph::add_hits_to_kmergraphs(const vector<LocalPRG *> &prgs) {
    uint num_hits[2];
    for (auto pnode : nodes) {
        // copy kmergraph
        pnode.second->kmer_prg = prgs[pnode.second->prg_id]->kmer_prg;
	assert(pnode.second->kmer_prg == prgs[pnode.second->prg_id]->kmer_prg);
        num_hits[0] = 0;
        num_hits[1] = 0;

        // add hits
        for (auto read : pnode.second->reads) {
            for (auto mh = read->hits[pnode.second->node_id].begin();
                 mh != read->hits[pnode.second->node_id].end(); ++mh) {
                //bool added = false;
                // update the covg in the kmer_prg
                pnode.second->kmer_prg.nodes[(*mh)->knode_id]->covg[(*mh)->strand] += 1;
                num_hits[(*mh)->strand] += 1;
		//cout << "add hit " << **mh << " to " << *pnode.second->kmer_prg.nodes[(*mh)->knode_id] << " which has path " << pnode.second->kmer_prg.nodes[(*mh)->knode_id]->path << endl;
                /*for (uint i=0; i!=pnode.second->kmer_prg.nodes.size(); ++i)
                {
                    if (pnode.second->kmer_prg.nodes[i]->path == (*mh)->prg_path)
                    {
                        pnode.second->kmer_prg.nodes[i]->covg[(*mh)->strand] += 1;
			num_hits[(*mh)->strand] += 1;
                        added = true;
                        break;
                    }
                }
                assert(added == true || assert_msg("could not find kmernode corresponding to " << **mh));*/
            }
        }
        //cout << now() << "Added " << num_hits[1] << " hits in the forward direction and " << num_hits[0]
        //     << " hits in the reverse" << endl;
        pnode.second->kmer_prg.num_reads = pnode.second->covg;
    }
}

bool PanGraph::operator==(const PanGraph &y) const {
    // false if have different numbers of nodes
    if (y.nodes.size() != nodes.size()) {
        cout << "different num nodes " << nodes.size() << "!=" << y.nodes.size() << endl;
        return false;
    }

    // false if have different numbers of edges
    if (y.edges.size() != edges.size()) {
        cout << "different num edges " << edges.size() << "!=" << y.edges.size() << endl;
        return false;
    }

    // false if have different nodes
    for (const auto c: nodes) {
        // if node id doesn't exist 
        auto it = y.nodes.find(c.first);
        if (it == y.nodes.end()) {
            cout << "can't find node " << c.first << endl;
            return false;
        }
    }
    // or different edges
    for (const auto e: edges) {
        pointer_values_equal<PanEdge> eq = {e};
        auto it = find_if(y.edges.begin(), y.edges.end(), eq);

        if (it == y.edges.end()) {
            cout << "couldn't find " << *e << endl;
            return false;
        }

    }
    // otherwise is true
    return true;
}

void PanGraph::construct_tuple_graph(uint tuple_size, TupleGraph& tg)
{
    for (auto r : reads)
    {
	for (uint i=0; i+tuple_size<r.second->edges.size(); ++i)
	{
	    vector<PanEdge*> e(r.second->edges.begin()+i, r.second->edges.begin()+i+tuple_size);
	    tg.add_tuple(e, r.second);
	}
    }
}

bool PanGraph::operator!=(const PanGraph &y) const {
    return !(*this == y);
}

void PanGraph::write_gfa(const string &filepath) {
    ofstream handle;
    handle.open(filepath);
    handle << "H\tVN:Z:1.0" << endl;
    for (auto &node : nodes) {

        handle << "S\t" << node.second->get_name() << "\tN\tFC:i:" << node.second->covg << endl;
    }

    for (uint i = 0; i != edges.size(); ++i) {
        if (edges[i]->orientation == 0) {
            handle << "L\t" << edges[i]->from->get_name() << "\t-\t" << edges[i]->to->get_name() << "\t-\t0M\tRC:i:"
                   << edges[i]->covg << endl;
        } else if (edges[i]->orientation == 1) {
            handle << "L\t" << edges[i]->from->get_name() << "\t+\t" << edges[i]->to->get_name() << "\t-\t0M\tRC:i:"
                   << edges[i]->covg << endl;
        } else if (edges[i]->orientation == 2) {
            handle << "L\t" << edges[i]->from->get_name() << "\t-\t" << edges[i]->to->get_name() << "\t+\t0M\tRC:i:"
                   << edges[i]->covg << endl;
        } else if (edges[i]->orientation == 3) {
            handle << "L\t" << edges[i]->from->get_name() << "\t+\t" << edges[i]->to->get_name() << "\t+\t0M\tRC:i:"
                   << edges[i]->covg << endl;
        }
    }
    handle.close();
}

void PanGraph::save_matrix(const string &filepath) {
    // write a presence/absence matrix for samples and nodes
    ofstream handle;
    handle.open(filepath);

    // save header line with sample names
    for (auto s : samples) {
        handle << "\t" << s.second->name;
    }
    handle << endl;

    // for each node, save number of each sample
    for (auto n : nodes) {
        handle << n.second->name;
        for (auto s : samples) {
            if (s.second->paths.find(n.second->node_id) == s.second->paths.end()) {
                handle << "\t0";
            } else {
                handle << "\t" << s.second->paths[n.second->node_id].size();
            }
        }
        handle << endl;
    }
}

std::ostream &operator<<(std::ostream &out, PanGraph const &m) {
    //cout << "printing pangraph" << endl;
    for (const auto &n : m.nodes) {
        cout << n.second->prg_id << endl;
    }
    for (const auto &e : m.edges) {
        cout << *e << endl;
    }
    return out;
}
