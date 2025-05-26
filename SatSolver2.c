#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_VARS 1000
#define MAX_CLAUSES 10000
#define MAX_VERTICES 100
#define MAX_EDGES 1000
#define MAX_COLORS 10

typedef struct {
    int *literals;
    int size;
} Clause;

typedef struct {
    Clause *clauses;
    int num_vars;
    int num_clauses;
} Formula;

typedef struct AssignNode {
    int var;
    int value;
    struct AssignNode *left;
    struct AssignNode *right;
    struct AssignNode *parent;
} AssignNode;

AssignNode* create_node(int var, int value, AssignNode *parent) {
    AssignNode *node = malloc(sizeof(AssignNode));
    node->var = var;
    node->value = value;
    node->left = node->right = NULL;
    node->parent = parent;
    return node;
}

int get_assignment(AssignNode *node, int var) {
    while (node) {
        if (node->var == var)
            return node->value;
        node = node->parent;
    }
    return 0;
}

bool eval_literal(AssignNode *node, int lit) {
    int var = abs(lit);
    int val = get_assignment(node, var);
    if (val == 0) return false;
    return (lit > 0 && val == 1) || (lit < 0 && val == -1);
}

bool is_clause_satisfied(Formula *F, Clause *clause, AssignNode *node) {
    for (int i = 0; i < clause->size; i++) {
        if (eval_literal(node, clause->literals[i])) return true;
    }
    return false;
}

bool is_clause_unsatisfiable(Formula *F, Clause *clause, AssignNode *node) {
    for (int i = 0; i < clause->size; i++) {
        int var = abs(clause->literals[i]);
        if (get_assignment(node, var) == 0)
            return false;
    }
    return !is_clause_satisfied(F, clause, node);
}

bool sat_tree(Formula *F, AssignNode *node, AssignNode **solution) {
    bool all_satisfied = true;
    for (int i = 0; i < F->num_clauses; i++) {
        if (is_clause_unsatisfiable(F, &F->clauses[i], node))
            return false;
        if (!is_clause_satisfied(F, &F->clauses[i], node))
            all_satisfied = false;
    }

    if (all_satisfied) {
        *solution = node;
        return true;
    }

    for (int i = 1; i <= F->num_vars; i++) {
        if (get_assignment(node, i) == 0) {
            node->right = create_node(i, 1, node);
            if (sat_tree(F, node->right, solution)) return true;
            node->left = create_node(i, -1, node);
            if (sat_tree(F, node->left, solution)) return true;
            return false;
        }
    }

    return false;
}

void free_formula(Formula *F) {
    for (int i = 0; i < F->num_clauses; i++) {
        free(F->clauses[i].literals);
    }
    free(F->clauses);
}

void free_tree(AssignNode *node) {
    if (!node) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

int var(int vertice, int cor, int total_cor) {
    return (vertice - 1) * total_cor + cor;
}

void print_coloring(AssignNode *node, int total_vert, int total_cor) {
    int result[MAX_VARS + 1] = {0};
    while (node && node->var != 0) {
        result[node->var] = node->value;
        node = node->parent;
    }

    for (int i = 1; i <= total_vert; i++) {
        for (int j = 1; j <= total_cor; j++) {
            int var_num = (i - 1) * total_cor + j;
            if (result[var_num] == 1) {
                printf("Vertice %d: Cor %d\n", i, j);
                break;
            }
        }
    }
}

void graph_to_cnf(const char *filename, Formula *F, int *V_out, int *K_out) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Erro ao abrir grafo.txt");
        exit(1);
    }

    int Vertice, Edge;
    fscanf(fp, "%d %d", &Vertice, &Edge);
    int edges[MAX_EDGES][2];
    for (int i = 0; i < Edge; i++) {
        fscanf(fp, "%d %d", &edges[i][0], &edges[i][1]);
    }

    int total_cor;
    printf("Informe o numero de cores: ");
    scanf("%d", &total_cor);

    F->num_vars = Vertice * total_cor;
    F->clauses = malloc(sizeof(Clause) * MAX_CLAUSES);
    F->num_clauses = 0;

    // 1. Cada vertice tem pelo menos uma cor
    for (int i = 1; i <= Vertice; i++) {
        Clause c;
        c.literals = malloc(sizeof(int) * total_cor);
        c.size = 0;
        for (int j = 1; j <= total_cor; j++) {
            c.literals[c.size++] = var(i, j, total_cor);
        }
        F->clauses[F->num_clauses++] = c;
    }

    // 2. Cada vertice tem no maximo uma cor
    for (int i = 1; i <= Vertice; i++) {
        for (int j = 1; j <= total_cor; j++) {
            for (int l = j + 1; l <= total_cor; l++) {
                Clause c;
                c.literals = malloc(sizeof(int) * 2);
                c.size = 2;
                c.literals[0] = -var(i, j, total_cor);
                c.literals[1] = -var(i, l, total_cor);
                F->clauses[F->num_clauses++] = c;
            }
        }
    }

    // 3. Vizinhos nao podem ter mesma cor
    for (int e = 0; e < Edge; e++) {
        int u = edges[e][0];
        int v = edges[e][1];
        for (int j = 1; j <= total_cor; j++) {
            Clause c;
            c.literals = malloc(sizeof(int) * 2);
            c.size = 2;
            c.literals[0] = -var(u, j, total_cor);
            c.literals[1] = -var(v, j, total_cor);
            F->clauses[F->num_clauses++] = c;
        }
    }

    fclose(fp);
    *V_out = Vertice;
    *K_out = total_cor;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s grafo.txt\n", argv[0]);
        return 1;
    }

    Formula F;
    int Vertice, cor;
    graph_to_cnf(argv[1], &F, &Vertice, &cor);

    AssignNode *root = create_node(0, 0, NULL);
    AssignNode *solution_node = NULL;

    bool satisfiable = sat_tree(&F, root, &solution_node);

    if (satisfiable && solution_node != NULL) {
        printf("SAT!\n");
        print_coloring(solution_node, Vertice, cor);
    } else {
        printf("UNSAT!\n");
    }

    free_tree(root);
    free_formula(&F);

    return 0;
}
