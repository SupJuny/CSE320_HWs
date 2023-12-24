#include <stdlib.h>

#include "global.h"
#include "debug.h"

/**
 * @brief  Read genetic distance data and initialize data structures.
 * @details  This function reads genetic distance data from a specified
 * input stream, parses and validates it, and initializes internal data
 * structures.
 *
 * The input format is a simplified version of Comma Separated Values
 * (CSV).  Each line consists of text characters, terminated by a newline.
 * Lines that start with '#' are considered comments and are ignored.
 * Each non-comment line consists of a nonempty sequence of data fields;
 * each field iis terminated either by ',' or else newline for the last
 * field on a line.  The constant INPUT_MAX specifies the maximum number
 * of data characters that may be in an input field; fields with more than
 * that many characters are regarded as invalid input and cause an error
 * return.  The first field of the first data line is empty;
 * the subsequent fields on that line specify names of "taxa", which comprise
 * the leaf nodes of a phylogenetic tree.  The total number N of taxa is
 * equal to the number of fields on the first data line, minus one (for the
 * blank first field).  Following the first data line are N additional lines.
 * Each of these lines has N+1 fields.  The first field is a taxon name,
 * which must match the name in the corresponding column of the first line.
 * The subsequent fields are numeric fields that specify N "distances"
 * between this taxon and the others.  Any additional lines of input following
 * the last data line are ignored.  The distance data must form a symmetric
 * matrix (i.e. D[i][j] == D[j][i]) with zeroes on the main diagonal
 * (i.e. D[i][i] == 0).
 *
 * If 0 is returned, indicating data successfully read, then upon return
 * the following global variables and data structures have been set:
 *   num_taxa - set to the number N of taxa, determined from the first data line
 *   num_all_nodes - initialized to be equal to num_taxa
 *   num_active_nodes - initialized to be equal to num_taxa
 *   node_names - the first N entries contain the N taxa names, as C strings
 *   distances - initialized to an NxN matrix of distance values, where each
 *     row of the matrix contains the distance data from one of the data lines
 *   nodes - the "name" fields of the first N entries have been initialized
 *     with pointers to the corresponding taxa names stored in the node_names
 *     array.
 *   active_node_map - initialized to the identity mapping on [0..N);
 *     that is, active_node_map[i] == i for 0 <= i < N.
 *
 * @param in  The input stream from which to read the data.
 * @return 0 in case the data was successfully read, otherwise -1
 * if there was any error.  Premature termination of the input data,
 * failure of each line to have the same number of fields, and distance
 * fields that are not in numeric format should cause a one-line error
 * message to be printed to stderr and -1 to be returned.
 */

void clean_buffer(char* buffer, int size) {
    for (int i = 0; i < size; i++) {
        *(buffer + i) = 0;
    }
}

double char_to_number(char* input_buffer, int size, int deci_size) {
    int count = 0;
    int flo = size;
    double power = 1.0;
    double decimal = 1.0;
    double value = 0.0;
    double final = 0.0;
    int temp = size;
    int i = 0;

    //printf("number size: %d \n", size);
    while (*(input_buffer+i) != '\0') {
        //printf("content of buffer: %c \n", *(input_buffer+i));
        //printf("next content of buffer: %d \n", *(input_buffer+i+1));
        temp = temp-1;
        count++;

        if (*(input_buffer+i) == '.') {
            //printf("period section \n");
            flo = flo - count;
            i++;
            continue;
        }

        if (*(input_buffer+i) < '0' || *(input_buffer+i) > '9'){
            //printf("it is not number \n");
            return -1;
        }

        if (flo != size) {
            //printf("decimal section \n");
            decimal = decimal * 0.1;
            value = *(input_buffer+i) - '0';
            value = value * decimal;
            i++;
        }
        else {
            if (temp == 0) {
                //printf("single digit \n");
                value = *(input_buffer+i) - '0';
                i++;
            }

            else {
                //printf("more than single digit and temp value is %d \n", temp);
                int j = count;
                if (deci_size == 1)
                    value = *(input_buffer+i) - '0';
                else {
                    while (j != 0) {
                        //printf("j value is %d \n", j);
                        power = power * 10.0;
                        j--;
                    }

                    value = *(input_buffer+i) - '0';
                    value = value * power;
                }

                //printf("power: %f, value: %f \n", power, value);
                i++;
            }
        }
        //printf("next buffer content %c \n", *(input_buffer+i));
        //printf("got final value \n");
        final = final + value;
    }
    //printf("final value: %f \n", final);
    clean_buffer(input_buffer, size);

    return final;
}

int read_distance_data(FILE *in) {
    // TO BE IMPLEMENTED

    char input = fgetc(in);
    //printf("this is the input \n");
    while (input != EOF) {
        if (input == '#') {
            while (input != '\n')
                input = fgetc(in);

            //printf("input1 is %c \n", input);
            input = fgetc(in);
            //printf("input2 is %c \n", input);
        }

        if (input == ',') {
            break;
        }
    }                                      // get input without comment lines
    int count = 0;
    int i = 0;

    //printf("value input is %d \n", input);

    while (input != '\n') {
        // if (input == '#') {
        //     while (input != '\n')
        //         input = fgetc(in);
        //     input = fgetc(in);
        //     printf("input is %d \n", input);
        // }
        // printf("passing first line \n");

        if (input == ',') {
            //printf("comma function  and input is %c \n", input);
            // while (input != '\n') {
            //     input = fgetc(in);
            // }
            input = fgetc(in);
            //printf("value is %d \n", input);
        }

        else if (input == '#') {
            while (input != '\n')
                input = fgetc(in);

            input = fgetc(in);
        }

        else {                                                  // collecting node name
            //printf("node name function \n");
            //printf("%c \n", input);
            count++;
            int j = 0;

            while (input != ','){
                if (input == '\n') {
                    //printf("new line is detected \n");
                    //input = fgetc(in);
                    break;
                }
                //printf("%c \n", input);
                *(*(node_names+i)+j) = input;
                input = fgetc(in);
                j++;
                if (j > INPUT_MAX)
                    return -1;
            }
            *(*(node_names+i)+j+1) = '\0';
            i++;

            if (i > MAX_NODES)
                return -1;
        }
        //printf("%c \n", input);
        //input = fgetc(in);
    }

    // checking node name has all names
    // printf("putting names is done \n");
    int j = 0;

    for (int i = 0; i < count; i++) {
        //printf("inside for loop \n");
        while (*(*(node_names+i)+j) != '\0') {
            //printf("this is node name[%d][%d]: %c \n", i, j, *(*(node_names+i)+j));
            j++;
        }
        j=0;
    }

    // printf("while loop for first line is done %d \n", count);
    input = fgetc(in);
    //printf("next input value is %d \n", input);

    int row_count = 0;

    int first_char_read = 0;
    int l = 0;

    while (input != EOF) {                                      // read each line
        //printf("wl 2 \n");
        int content_count = 0;

        if (first_char_read == 0) {
            //printf("first char \n");
            int i=0;
            //printf("input 1 value is %c \n", input);
            while (input != ',') {                              // read name field & checking names on column and row are same
                if (input == '#') {
                    while (input != '\n')
                        input = fgetc(in);

                    input =fgetc(in);
                }
                first_char_read = 1;
                //sprintf("input: %c node_names: %c \n", input, *(*(node_names+row_count)+content_count));

                *(input_buffer+i)= input;
                i++;

                input = fgetc(in);
                //printf("input 2 value is %c \n", input);
            }

            while (*(*(node_names+row_count)+content_count) != '\0') {
                if(*(input_buffer+content_count) != *(*(node_names+row_count)+content_count)) {
                    // printf("name is not symmetric \n");
                    // printf("this is node name[%d][%d]: %c \n", row_count, content_count, *(*(node_names+row_count)+content_count));
                    // printf("this is buffer name[%d]: %c \n", content_count, *(input_buffer+content_count));
                    return -1;
                }
                else {
                    // printf("name is matched well \n");
                    // printf("this is node name[%d][%d]: %c \n", row_count, content_count, *(*(node_names+row_count)+content_count));
                    // printf("this is buffer name[%d]: %c \n", content_count, *(input_buffer+content_count));
                    content_count++;
                }
            }
            input = fgetc(in);
        }
        else {

            // printf("distance check and current input: %c \n", (char)input);

            int size = 0;
            int i=0;
            double dist = 0.0;
            int period_check = 0;
            int deci_size = 0;

            while (input != ',' && input != '\n') {
                *(input_buffer+i) = input;
                if (input == '.')
                    period_check = 1;
                if (period_check == 0)
                    deci_size++;
                i++;
                size++;
                input = fgetc(in);
            }
            *(input_buffer+i) = '\0';
            // printf("input buffer is %c \n", *input_buffer);
            dist = char_to_number(input_buffer, size, deci_size);
            *(*(distances+row_count)+l) = dist;
            //printf(" distance[%d][%d] = %f \n", row_count, l, *(*(distances+row_count)+l));
            if (input == '\n'){
                first_char_read = 0;
                row_count++;
                l = -1;
                //break;
            }
            input = fgetc(in);
            l++;
        }

    }
    row_count++;

    num_taxa = count;
    num_all_nodes = count;
    num_active_nodes = count;

    // symmetric checking
    int a = 0;
    int b = 0;
    while (a != num_taxa) {
        //printf("a: %d b: %d \n", a, b);
        //printf("distance[%d][%d]: %f == distances[%d][%d]: %f \n", a,b,*(*(distances+a)+b),b,a,*(*(distances+b)+a));
        if (*(*(distances+a)+b) == *(*(distances+b)+a)) {
            b++;
            if (b == num_taxa) {
                //printf("%d line is symmetric \n", a);
                a++;
                b = 0;
            }
        }

        else{
            //printf("not symmetric \n");
            return -1;
        }

    }

    return 0;
}

/**
 * @brief  Emit a representation of the phylogenetic tree in Newick
 * format to a specified output stream.
 * @details  This function emits a representation in Newick format
 * of a synthesized phylogenetic tree to a specified output stream.
 * See (https://en.wikipedia.org/wiki/Newick_format) for a description
 * of Newick format.  The tree that is output will include for each
 * node the name of that node and the edge distance from that node
 * its parent.  Note that Newick format basically is only applicable
 * to rooted trees, whereas the trees constructed by the neighbor
 * joining method are unrooted.  In order to turn an unrooted tree
 * into a rooted one, a root will be identified according by the
 * following method: one of the original leaf nodes will be designated
 * as the "outlier" and the unique node adjacent to the outlier
 * will serve as the root of the tree.  Then for any other two nodes
 * adjacent in the tree, the node closer to the root will be regarded
 * as the "parent" and the node farther from the root as a "child".
 * The outlier node itself will not be included as part of the rooted
 * tree that is output.  The node to be used as the outlier will be
 * determined as follows:  If the global variable "outlier_name" is
 * non-NULL, then the leaf node having that name will be used as
 * the outlier.  If the value of "outlier_name" is NULL, then the
 * leaf node having the greatest total distance to the other leaves
 * will be used as the outlier.
 *
 * @param out  Stream to which to output a rooted tree represented in
 * Newick format.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.  If the global variable "outlier_name" is
 * non-NULL, then it is an error if no leaf node with that name exists
 * in the tree.
 */

double get_distance_from_node (NODE* nod1, NODE* nod2) {
    int i = 0;
    int index = 0;
    int jndex = 0;
    double gdfn= 0.0;

    while (i != num_all_nodes) {
        if(*(node_names + i) == nod1->name) {
            index = i;
            break;
        }
        i++;
    }

    i = 0;
    while (i != num_all_nodes) {
        if(*(node_names + i) == nod2->name) {
            jndex = i;
            break;
        }
        i++;
    }

    gdfn = *(*(distances + index) + jndex);
    return gdfn;
}



int node_change_count = 0;
int node_count = 0;
int have_sibling = 1;

// recursive function to get each node's distance
void compare_two(NODE* node1, NODE* node2, FILE* out) {
    // printf("ww1 change count is %d \n", node_change_count);
    // printf("node1 name is  %s \n", node1->name);
    // printf("node2 name is  %s \n", node2->name);
    //printf("(");
    // NODE* node3 = node1;
    // NODE* node4 = node2;
    //printf("%s, ", node1->name);

    int i = 0;
    node_change_count = 0;
    double newick_dist = 0.0;

    while (i != 3) {
        // printf("num of sibling 0: %d \n", have_sibling);
        //printf("ww2 change count is %d \n", node_change_count);

        if (*(node1->neighbors + i) == NULL) {
            //printf("%s neighbor %d is NULL \n", node1->name, i);
            node_change_count++;
            i++;

            continue;
        }

        else if (*(node1->neighbors + i) == node2) {
            //printf("%s neighbor %d is same with previous node \n", node1->name, i);
            node_change_count++;
            i++;

            continue;
        }

        else {
            //printf("%s neighbor %d is the first appeared node \n", node1->name, i);
            NODE* node3 = *(node1->neighbors + i);
            NODE* node4 = node1;
            have_sibling++;

            if (have_sibling == 1)
                fprintf(out, ",");
            else
                fprintf(out, "(");

            // printf("num of sibling 1: %d \n", have_sibling);

            compare_two(node3, node4, out);
        }
        //printf("ww3 change count is %d \n", node_change_count);
        // printf("num of sibling 2: %d \n", have_sibling);

        i++;
    }

    newick_dist = get_distance_from_node(node1, node2);

    // printf("num of sibling 3: %d \n", have_sibling);

    if (have_sibling == 0) {
        fprintf(out, ")");
    }

    if (node_change_count == 3) {
        node_count ++;
        have_sibling = 0;
    }

    //printf("num of sibling 4: %d \n", have_sibling);
    //printf("ww4 change count is %d \n", node_change_count);

    fprintf(out, "%s:%.2f", node1->name, newick_dist);
}



int emit_newick_format(FILE *out) {
    // TO BE IMPLEMENTED
    int valid_outlier = 0;
    int outlier_index = 0;
    int i = 0;
    int j = 0;
    double temp = 0.0;
    double outlier_val = 0.0;

    // valid_outlier = out_num_count;

    // -o : outlier is defined              ---- need to fix ----
    if (outlier_name != NULL) {
        int out_num_count = 0;
        while (*(outlier_name + j) != '\0') {
            out_num_count++;
            j++;
        }

        // printf("num of taxa is %d \n", num_taxa);
        // printf("outlier is defined: %c \n", *outlier_name);
        while (i != num_taxa) {
            j = 0;
            // printf("hi1 \n");
            while (*(*(node_names + i) + j) != '\0') {
                // printf("hi2 \n");
                if (*(outlier_name + j) == *(*(node_names + i) + j)) {
                    // printf("hi3 \n");
                    valid_outlier++;
                    j++;
                    if (valid_outlier == out_num_count) {
                        outlier_index = i;
                    }
                    continue;
                }

                else {
                    //printf("hi4 \n");
                    break;
                }
            }
            // printf("increase i, %d \n", i);
            i++;
            // j = 0;
            // while (*(outlier_name + j) != '\0') {
            //     if (*(outlier_name + j) == *(*(node_names + i) + j))
            //         valid_outlier = 1;
            //     j++;
            // }
            // i++;
        }
        //printf("hi2 \n");
        //printf("valid_outlier is %d\n", valid_outlier);
        if (valid_outlier != out_num_count)
            return -1;

    }


    // finding defualt outlier
    else {
        //printf("num taxa is %d \n", num_taxa);
        i = 0;
        j = 1;
        while (i != num_taxa) {
            //printf("wl 1 \n");
            while (j != num_taxa) {
                //printf("wl2 \n");
                temp = *(*(distances + i) + j);
                if (outlier_val < temp) {
                    outlier_val = temp;
                    outlier_index = i;
                }
                j++;
            }
            i++;
            j = i + 1;
        }
    }

    //printf("\n outlier index is %d \n", outlier_index);


    // make newick
    NODE* node_1 = nodes + outlier_index;
    NODE* node_2 = *(node_1->neighbors+0);
    //NODE* node_2 = *(node_1->neighbors+0);
    //printf("\n outlier name is %s \n", node_1->name);

    compare_two(node_2, node_1, out);
    printf("\n");

    return 0;
}

/**
 * @brief  Emit the synthesized distance matrix as CSV.
 * @details  This function emits to a specified output stream a representation
 * of the synthesized distance matrix resulting from the neighbor joining
 * algorithm.  The output is in the same CSV form as the program input.
 * The number of rows and columns of the matrix is equal to the value
 * of num_all_nodes at the end of execution of the algorithm.
 * The submatrix that consists of the first num_leaves rows and columns
 * is identical to the matrix given as input.  The remaining rows and columns
 * contain estimated distances to internal nodes that were synthesized during
 * the execution of the algorithm.
 *
 * @param out  Stream to which to output a CSV representation of the
 * synthesized distance matrix.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.
 */
int emit_distance_matrix(FILE *out) {
    // TO BE IMPLEMENTED
    int i = 0;
    while (i != num_all_nodes) {
        int j = 0;
        fprintf(out, ",");
        while (*(*(node_names + i) + j) != '\0') {
            fprintf(out, "%c", *(*(node_names + i) + j));
            j++;
        }
        i++;
    }
    fprintf(out, "\n");

    int frist_char = 0;
    i = 0;
    while (i != num_all_nodes) {
        if (frist_char == 0) {
            int j = 0;
            while (*(*(node_names + i) + j) != '\0') {
                fprintf(out, "%c", *(*(node_names + i) + j));
                j++;
            }
        }

        int j = 0;
        while (j != num_all_nodes) {
            fprintf(out, ",%.2f", *(*(distances + i) + j));
            j++;
        }
        i++;
        frist_char = 0;
        fprintf(out, "\n");

    }
    return 0;
}

/**
 * @brief  Build a phylogenetic tree using the distance data read by
 * a prior successful invocation of read_distance_data().
 * @details  This function assumes that global variables and data
 * structures have been initialized by a prior successful call to
 * read_distance_data(), in accordance with the specification for
 * that function.  The "neighbor joining" method is used to reconstruct
 * phylogenetic tree from the distance data.  The resulting tree is
 * an unrooted binary tree having the N taxa from the original input
 * as its leaf nodes, and if (N > 2) having in addition N-2 synthesized
 * internal nodes, each of which is adjacent to exactly three other
 * nodes (leaf or internal) in the tree.  As each internal node is
 * synthesized, information about the edges connecting it to other
 * nodes is output.  Each line of output describes one edge and
 * consists of three comma-separated fields.  The first two fields
 * give the names of the nodes that are connected by the edge.
 * The third field gives the distance that has been estimated for
 * this edge by the neighbor-joining method.  After N-2 internal
 * nodes have been synthesized and 2*(N-2) corresponding edges have
 * been output, one final edge is output that connects the two
 * internal nodes that still have only two neighbors at the end of
 * the algorithm.  In the degenerate case of N=1 leaf, the tree
 * consists of a single leaf node and no edges are output.  In the
 * case of N=2 leaves, then no internal nodes are synthesized and
 * just one edge is output that connects the two leaves.
 *
 * Besides emitting edge data (unless it has been suppressed),
 * as the tree is built a representation of it is constructed using
 * the NODE structures in the nodes array.  By the time this function
 * returns, the "neighbors" array for each node will have been
 * initialized with pointers to the NODE structure(s) for each of
 * its adjacent nodes.  Entries with indices less than N correspond
 * to leaf nodes and for these only the neighbors[0] entry will be
 * non-NULL.  Entries with indices greater than or equal to N
 * correspond to internal nodes and each of these will have non-NULL
 * pointers in all three entries of its neighbors array.
 * In addition, the "name" field each NODE structure will contain a
 * pointer to the name of that node (which is stored in the corresponding
 * entry of the node_names array).
 *
 * @param out  If non-NULL, an output stream to which to emit the edge data.
 * If NULL, then no edge data is output.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.
 */

double calc_row_sum(int i, int num_active) {
    int j = 0;
    double sum = 0.0;
    //printf("lets do row sum for %d row \n", i);
    while (j != num_active) {
        //printf("*(active_node_map+j) is %d \n", *(active_node_map+j));
        // if (*(active_node_map+j) == -1) {
        //     j++;
        //     continue;
        // }
        if (*(active_node_map+j) == -2)
            break;

        sum += *(*(distances + i) + *(active_node_map+j));

        //printf("each sum is %f \n", sum);
        j++;
    }
    //printf("sum is %f \n", sum);
    return sum;
}

double dist_new_to_rest(int ind_i, int ind_j, int rest) {
    double dist = 0.0;

    dist = (*(*(distances + ind_i) + rest) + *(*(distances + ind_j) + rest) - *(*(distances + ind_i) + ind_j)) / 2;

    //printf("i is %d, j is %d, dist is %f \n", ind_i, ind_j, dist);

    return dist;
}

int build_taxonomy(FILE *out) {
    // TO BE IMPLEMENTED

    // put data into active_node_name
    int s = 0;

    while (s != num_taxa) {
        *(active_node_map+s) = s;
        //printf("active_node_map[%d] is %d \n", s, *(active_node_map+s));
        s++;

        //printf("active_node_map[%d] is %d \n", s, *(active_node_map+s));
    }
    *(active_node_map+ (s)) = -2;
    //printf("active_node_map[%d] is %d \n", s, *(active_node_map+(s)));


    //printf("num_active_nodes = %d \n", num_active_nodes);
    while (num_active_nodes != 2) {

        //int node_count = 0;

        int index_i = 0;
        int index_j = 0;

        int actual_i = 0;
        int actual_j = 0;

        //printf("num al node: %d \n", num_all_nodes);

        int i = 0;
        int j = 1;

        double q_val = 0.0;
        double temp = 0.0;
        //printf("get in to get_j_by_q function \n");
        //printf("active_node_map is %d \n", *(active_node_map + i));

        while (*(active_node_map + i) != -2) {

            while (*(active_node_map + j) != -2) {
                //printf("wl 2 \n");
                if (i == j) {
                    j++;
                    continue;
                }

                temp = ((num_active_nodes-2) * (*(*(distances + *(active_node_map + i)) + *(active_node_map + j))) - calc_row_sum(*(active_node_map + i), num_active_nodes) - calc_row_sum(*(active_node_map + j), num_active_nodes));           // calculating Q value
                // printf("(n-2) = %d, d(%d,%d) = %f, row_sum(%d) = %f, row_sum(%d) = %f \n", (num_active_nodes-2), *(active_node_map + i), *(active_node_map + j), (*(*(distances+*(active_node_map + i))+*(active_node_map + j))), *(active_node_map + i), calc_row_sum(*(active_node_map + i), num_all_nodes), *(active_node_map + j), calc_row_sum(*(active_node_map + j), num_all_nodes));
                // printf("temp value is %f and index is (%d,%d) \n\n", temp, *(active_node_map + i), *(active_node_map + j));
                if (q_val > temp) {
                    q_val = temp;
                    index_i = i;
                    index_j = j;
                    actual_i = *(active_node_map + i);
                    actual_j = *(active_node_map + j);
                    j++;
                }
                else
                    j++;
            }

            i++;
            j = i;
        }

        // printf("index_i is %d \n", actual_i);
        // printf("index_j is %d \n", actual_j);

        double dist_i_to_new = ((*(*(distances+ actual_i) + actual_j) / 2)) + (((calc_row_sum(actual_i, num_active_nodes) - calc_row_sum(actual_j, num_active_nodes)) / 2) / (num_active_nodes-2));
        double dist_j_to_new = (*(*(distances+ actual_i) + actual_j)) - dist_i_to_new;
        // printf("distance from i to new: %f \n", dist_i_to_new);
        // printf("distance from j to new: %f \n", dist_j_to_new);

        int new_node_name = num_all_nodes;
        int int_char = 0;
        int buffer_index = 0;
        int digit = 4;

        *(input_buffer + buffer_index) = '#';
        buffer_index++;

        while(digit != 0){
            if (new_node_name >= 1000) {
                //printf("digit 4 \n");
                int_char = new_node_name / 1000;
                *(input_buffer + buffer_index) = int_char + '0';
                new_node_name = new_node_name % 1000;
                buffer_index++;
                digit = 3;
            }
            else if (new_node_name >= 100 || digit == 3) {
                //printf("digit 3 \n");
                int_char = new_node_name / 100;
                *(input_buffer + buffer_index) = int_char + '0';
                new_node_name = new_node_name % 100;
                buffer_index++;
                digit = 2;
            }
            else if (new_node_name >= 10 || digit == 2) {
                //printf("digit 2 \n");
                int_char = new_node_name / 10;
                *(input_buffer + buffer_index) = int_char + '0';
                new_node_name = new_node_name % 10;
                buffer_index++;
                digit = 1;
            }
            else {
                //printf("digit 1 \n");
                *(input_buffer + buffer_index) = new_node_name + '0';
                buffer_index++;
                digit = 0;
                *(input_buffer + buffer_index) = '\0';
                //printf("%d \n", digit);
            }
        }

        num_all_nodes += 1;                                             // adding new node
        num_active_nodes -= 1;                                          // num_active_nodes + new node - 2(neighbors)

        *(active_node_map + index_i) = num_all_nodes-1;                              // deactivate node
        *(active_node_map + index_j) = *(active_node_map + (num_active_nodes));                              // deactivate node
        *(active_node_map + num_active_nodes) = -2;

        int p = 0;
        //printf("new input node name is ");
        while (*(input_buffer + p) != '\0') {               // put new node name is to the node_name array
            //printf("%c", *(input_buffer + p));
            *(*(node_names+ (num_all_nodes-1))+p) = *(input_buffer + p);
            //printf("%c", *(*(node_names + (num_all_nodes-1))+p));
            p++;
        }


        // put distances into the distance matrix
        *(*(distances + (num_all_nodes-1))+ actual_i) = dist_i_to_new;
        *(*(distances + actual_i)+ (num_all_nodes-1)) = dist_i_to_new;
        *(*(distances + (num_all_nodes-1))+ actual_j) = dist_j_to_new;
        *(*(distances + actual_j)+ (num_all_nodes-1)) = dist_j_to_new;


        // put data in the node structure
        (nodes + actual_i)->name = *(node_names + actual_i);
        (nodes + actual_j)->name = *(node_names + actual_j);
        (nodes + (num_all_nodes-1))->name = *(node_names + (num_all_nodes-1));
        *((nodes + actual_i)->neighbors + 0) = (nodes + (num_all_nodes-1));
        *((nodes + actual_j)->neighbors + 0) = (nodes + (num_all_nodes-1));
        *((nodes + (num_all_nodes-1))->neighbors + 1) = (nodes + actual_i);
        *((nodes + (num_all_nodes-1))->neighbors + 2) = (nodes + actual_j);

        // printf("%d node's name is  %s \n", actual_i, (nodes + actual_i)->name);
        // printf("%d node's name is  %s \n", actual_j, (nodes + actual_j)->name);
        // printf("%d node's name is  %s \n", (num_all_nodes-1), (nodes + (num_all_nodes-1))->name);

        // printf("%d node's child 1 name is  %s \n", (num_all_nodes-1), ((nodes + (num_all_nodes-1))->neighbors[1])->name);
        // printf("%d node's child 2 name is  %s \n", (num_all_nodes-1), ((nodes + (num_all_nodes-1))->neighbors[2])->name);

        // printf("%d node's parent name is  %s \n", actual_i, ((nodes + actual_i)->neighbors[0])->name);
        // printf("%d node's parent name is  %s \n", actual_j, ((nodes + actual_j)->neighbors[0])->name);


        // no flags print child node and parent node with distance
        if (global_options == 0) {
            fprintf(out, "%d,%d,%.2f\n", actual_i, (num_all_nodes -1), dist_i_to_new);
            fprintf(out, "%d,%d,%.2f\n", actual_j, (num_all_nodes -1), dist_j_to_new);
        }


        // calculate distance to rest
        int t = 0;
        while (t != num_all_nodes-1) {
            //printf("t = %d \n", t);
            if (t == actual_i || t == actual_j) {
                //printf("t = index_i or index j \n");
                t++;
                continue;
            }

            int z = 0;
            while (z != num_active_nodes) {
                if (*(active_node_map + z) != t) {
                    //printf("t: %d is not equal to z: %d \n", t, z);
                    z++;
                    continue;
                }
                else {
                    //printf("t: %d is equal to z: %d \n", t, z);
                    break;
                }
            }

            if (z == num_active_nodes) {
                //printf("z: %d is equal to num_active_nodes: %d \n", z, num_active_nodes);

                t++;
                continue;
            }

            else {
                //printf("num_active_nodes is %d \n", num_active_nodes);
                if (num_active_nodes == 2) {
                    // printf("num of active node is 2 \n");
                    // printf("deactivate node is %d and %d \n", actual_i, actual_j);
                    *(*(distances + *(active_node_map)) + *(active_node_map + 1)) = *(*(distances + (num_all_nodes - 2)) + *(active_node_map + 1)) - *(*(distances + (num_all_nodes - 2)) + *(active_node_map));
                    *(*(distances + *(active_node_map + 1)) + *(active_node_map)) = *(*(distances + *(active_node_map)) + *(active_node_map + 1));
                    //printf("distances[%d][%d] is %f \n", num_all_nodes-1, t, *(*(distances + (num_all_nodes-1))+ t));


                    // put last node's data in node structure
                    int last_i = *(active_node_map);
                    int last_j = *(active_node_map + 1);

                    (nodes + last_j)->name = *(node_names + last_j);
                    *((nodes + last_i)->neighbors + 0) = (nodes + last_j);
                    *((nodes + last_j)->neighbors + 0) = (nodes + last_i);
                    *((nodes + (num_all_nodes-1))->neighbors + 1) = (nodes + actual_i);

                    // printf("%d node's name is  %s \n", last_j, (nodes + last_j)->name);
                    // printf("%d node's name is  %s \n", last_i, (nodes + last_i)->name);
                    // //printf("%d node's name is  %s \n", (num_all_nodes-1), (nodes + (num_all_nodes-1))->name);

                    // printf("%d node's child 1 name is  %s \n", last_i, ((nodes + last_i)->neighbors[1])->name);
                    // printf("%d node's child 2 name is  %s \n", last_i, ((nodes + last_i)->neighbors[2])->name);

                    // printf("%d node's parent name is  %s \n", last_i, ((nodes + last_i)->neighbors[0])->name);
                    // printf("%d node's parent name is  %s \n", last_j, ((nodes + last_j)->neighbors[0])->name);


                    // no flags print child node and parent node with distance
                    if (global_options == 0) {
                        fprintf(out, "%d,%d,%.2f\n", t, (num_all_nodes -1), *(*(distances + *(active_node_map)) + *(active_node_map + 1)));
                    }
                    break;
                }
                else {
                    //printf("deactivate node is %d and %d \n", actual_i, actual_j);
                    *(*(distances + (num_all_nodes-1))+ t) = dist_new_to_rest(actual_i, actual_j, t);
                    *(*(distances + t)+ (num_all_nodes-1)) = *(*(distances + (num_all_nodes-1))+ t);
                    //printf("distances[%d][%d] is %f \n", num_all_nodes-1, t, *(*(distances + (num_all_nodes-1))+ t));
                    t++;
                }

            }

        }

        // int r = 0;
        // while (r != num_active_nodes+1) {
        //     printf("active node map: %d ", *(active_node_map+r));
        //     r++;
        // }
        // printf("\n");

    }

    // int p = 0;
    // int t=0;
    // while (t != num_all_nodes) {
    //     //printf("distances[%d][%d] is %f \n", t,p,*(*(distances + t)+ p));
    //     p++;
    //     if(p == num_all_nodes) {
    //         p=0;
    //         t++;
    //     }

    // }

    return 0;
}
