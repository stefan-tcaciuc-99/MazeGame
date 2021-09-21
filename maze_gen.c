#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WALL 'w'
#define POTION '#'
#define NEEDED_POTIONS 3
struct maze {
    char **a; // 2D array supporting maze
    unsigned int w; // width
    unsigned int h; // height
    unsigned int cell_size; // number of chars per cell; walls are 1 char
};

/**
 * Represents a cell in the 2D matrix.
 */
struct cell {
    unsigned int x;
    unsigned int y;
};

/**
 * Stack structure using a list of cells.
 * At element 0 in the list we have NULL.
 * Elements start from 1 onwards.
 * top_of_stack represents the index of the top of the stack
 * in the cell_list.
 */
struct stack {
    struct cell *cell_list;
    unsigned int top_of_stack;
    unsigned int capacity;
};

/**
 * Initialises the stack by allocating memory for the internal list
 */
void init_stack(struct stack *stack, unsigned int capacity) {
    stack->cell_list = (struct cell *) malloc(sizeof(struct cell) * (capacity + 1));
    stack->top_of_stack = 0;
    stack->capacity = capacity;
}

void free_stack(struct stack *stack) {
    free(stack->cell_list);
}

/**
 * Returns the element at the top of the stack and removes it
 * from the stack.
 * If the stack is empty, returns NULL
 */
struct cell stack_pop(struct stack *stack) {
    struct cell cell = stack->cell_list[stack->top_of_stack];
    if (stack->top_of_stack > 0) stack->top_of_stack--;
    return cell;
}

/**
 * Pushes an element to the top of the stack.
 * If the stack is already full (reached capacity), returns -1.
 * Otherwise returns 0.
 */
int stack_push(struct stack *stack, struct cell cell) {
    if (stack->top_of_stack == stack->capacity) return -1;
    stack->top_of_stack++;
    stack->cell_list[stack->top_of_stack] = cell;
    return 0;
}

int stack_isempty(struct stack *stack) {
    return stack->top_of_stack == 0;
}

//-----------------------------------------------------------------------------

void mark_visited(struct maze *maze, struct cell cell) {
    maze->a[cell.y][cell.x] = 'v';
}

/**
 * Convert a cell coordinate to a matrix index.
 * The matrix also contains the wall elements and a cell might span
 * multiple matrix cells.
 */
int cell_to_matrix_idx(struct maze *m, int cell) {
    return (m->cell_size + 1) * cell + (m->cell_size / 2) + 1;
}

/**
 * Convert maze dimension to matrix dimension.
 */
int maze_dimension_to_matrix(struct maze *m, int dimension) {
    return (m->cell_size + 1) * dimension + 1;
}

/**
 * Returns the index of the previous cell (cell - 1)
 */
int matrix_idx_prev_cell(struct maze *m, int cell_num) {
    return cell_num - (m->cell_size + 1);
}

/**
 * Returns the index of the next cell (cell + 1)
 */
int matrix_idx_next_cell(struct maze *m, int cell_num) {
    return cell_num + (m->cell_size + 1);
}

/**
 * Returns into neighbours the unvisited neighbour cells of the given cell.
 * Returns the number of neighbours.
 * neighbours must be able to hold 4 cells.
 */
int get_available_neighbours(struct maze *maze, struct cell cell, struct cell *neighbours) {
    int num_neighbrs = 0;

    // Check above
    if ((cell.y > cell_to_matrix_idx(maze, 0)) && (maze->a[matrix_idx_prev_cell(maze, cell.y)][cell.x] != 'v')) {
        neighbours[num_neighbrs].x = cell.x;
        neighbours[num_neighbrs].y = matrix_idx_prev_cell(maze, cell.y);
        num_neighbrs++;
    }

    // Check left
    if ((cell.x > cell_to_matrix_idx(maze, 0)) && (maze->a[cell.y][matrix_idx_prev_cell(maze, cell.x)] != 'v')) {
        neighbours[num_neighbrs].x = matrix_idx_prev_cell(maze, cell.x);
        neighbours[num_neighbrs].y = cell.y;
        num_neighbrs++;
    }

    // Check right
    if ((cell.x < cell_to_matrix_idx(maze, maze->w - 1)) &&
        (maze->a[cell.y][matrix_idx_next_cell(maze, cell.x)] != 'v')) {
        neighbours[num_neighbrs].x = matrix_idx_next_cell(maze, cell.x);
        neighbours[num_neighbrs].y = cell.y;
        num_neighbrs++;
    }

    // Check below
    if ((cell.y < cell_to_matrix_idx(maze, maze->h - 1)) &&
        (maze->a[matrix_idx_next_cell(maze, cell.y)][cell.x] != 'v')) {
        neighbours[num_neighbrs].x = cell.x;
        neighbours[num_neighbrs].y = matrix_idx_next_cell(maze, cell.y);
        num_neighbrs++;
    }

    return num_neighbrs;
}


/**
 * Removes a wall between two cells.
 */
void remove_wall(struct maze *maze, struct cell a, struct cell b) {
    int i;
    if (a.y == b.y) {
        for (i = 0; i < maze->cell_size; i++)
            maze->a[a.y - maze->cell_size / 2 + i][a.x - (((int) a.x - (int) b.x)) / 2] = ' ';
    } else {
        for (i = 0; i < maze->cell_size; i++)
            maze->a[a.y - (((int) a.y - (int) b.y)) / 2][a.x - maze->cell_size / 2 + i] = ' ';
    }
}

/**
 * Fill all matrix elements corresponding to the cell
 */
void fill_cell(struct maze *maze, struct cell c, char value) {
    int i, j;
    for (i = 0; i < maze->cell_size; i++)
        for (j = 0; j < maze->cell_size; j++)
            maze->a[c.y - maze->cell_size / 2 + i][c.x - maze->cell_size / 2 + j] = value;
}

/**
 * This function generates a maze of width x height cells.
 * Each cell is a square of cell_size x cell_size characters.
 * The maze is randly generated based on the supplied rand_seed.
 * Use the same rand_seed value to obtain the same maze.
 *
 * The function returns a struct maze variable containing:
 * - the maze represented as a 2D array (field a)
 * - the width (number of columns) of the array (field w)
 * - the height (number of rows) of the array (field h).
 * In the array, walls are represented with a 'w' character, while
 * pathways are represented with spaces (' ').
 * The edges of the array consist of walls, with the exception
 * of two openings, one on the left side (column 0) and one on
 * the right (column w-1) of the array. These should be used
 * as entry and exit.
 */
struct maze generate_maze(unsigned int width, unsigned int height, unsigned int cell_size, int rand_seed) {
    int row, col, i;
    struct stack stack;
    struct cell cell;
    struct cell neighbours[4];  // to hold neighbours of a cell
    int num_neighbs;
    struct maze maze;
    maze.w = width;
    maze.h = height;
    maze.cell_size = cell_size;
    maze.a = (char **) malloc(sizeof(char *) * maze_dimension_to_matrix(&maze, height));

    // Initialise RNG
    srand(rand_seed);

    // Initialise stack
    init_stack(&stack, width * height);

    // Initialise the matrix with walls
    for (row = 0; row < maze_dimension_to_matrix(&maze, height); row++) {
        maze.a[row] = (char *) malloc(maze_dimension_to_matrix(&maze, width));
        memset(maze.a[row], WALL, maze_dimension_to_matrix(&maze, width));
    }

    // Select a rand position on a border.
    // Border means x=0 or y=0 or x=2*width+1 or y=2*height+1
    cell.x = cell_to_matrix_idx(&maze, 0);
    cell.y = cell_to_matrix_idx(&maze, rand() % height);
    mark_visited(&maze, cell);
    stack_push(&stack, cell);

    while (!stack_isempty(&stack)) {
        // Take the top of stack
        cell = stack_pop(&stack);
        // Get the list of non-visited neighbours
        num_neighbs = get_available_neighbours(&maze, cell, neighbours);
        if (num_neighbs > 0) {
            struct cell next;
            // Push current cell on the stack
            stack_push(&stack, cell);
            // Select one rand neighbour
            next = neighbours[rand() % num_neighbs];
            // Mark it visited
            mark_visited(&maze, next);
            // Break down the wall between the cells
            remove_wall(&maze, cell, next);
            // Push new cell on the stack
            stack_push(&stack, next);
        }
    }

    // Finally, replace 'v' with spaces
    for (row = 0; row < maze_dimension_to_matrix(&maze, height); row++)
        for (col = 0; col < maze_dimension_to_matrix(&maze, width); col++)
            if (maze.a[row][col] == 'v') {
                cell.y = row;
                cell.x = col;
                fill_cell(&maze, cell, ' ');
            }

    // Select an entry point in the top right corner.
    // The first border cell that is available.
    for (row = 0; row < maze_dimension_to_matrix(&maze, height); row++)
        if (maze.a[row][1] == ' ') {
            maze.a[row][0] = ' ';
            break;
        }

    // Select the exit point
    for (row = maze_dimension_to_matrix(&maze, height) - 1; row >= 0; row--)
        if (maze.a[row][cell_to_matrix_idx(&maze, width - 1)] == ' ') {
            maze.a[row][maze_dimension_to_matrix(&maze, width) - 1] = ' ';
            break;
        }

    maze.w = maze_dimension_to_matrix(&maze, maze.w);
    maze.h = maze_dimension_to_matrix(&maze, maze.h);

    // Add the potions inside the maze at three rand locations
    for (i = 0; i < NEEDED_POTIONS; i++) {
        do {
            row = rand() % (maze.h - 1);
            col = rand() % (maze.w - 1);
        } while (maze.a[row][col] != ' ');
        maze.a[row][col] = POTION;
    }

    return maze;
}

//Main Function//Stefan Tcaciuc 
int main() {
    //Declaring Variables
    unsigned int w;
    unsigned int h;
    unsigned int cell;
    int seed;
    int x = 0;
    int y = 1;
    int fog = 0;
    char input = 0;
    int potion = 0;

    printf("Enter width:");
    scanf(" %d", &w);
    printf("Enter height:");
    scanf(" %d", &h);
    printf("Enter cell size:");
    scanf(" %d", &cell);
    printf("Enter Seed:");
    scanf(" %d", &seed);
    printf("Enter Fog:");
    scanf(" %d", &fog);

    //Calling The Maze
    struct maze maze = generate_maze(w, h, cell, seed);



    //While loop to keep looping while user has not left the maze
    while (x != maze.w-1 && y != maze.h - 1) {

        if (maze.a[y][x] == '#') { potion++; }//If statement to increment potions if user has collected a potion
        maze.a[y][x] = '@';//Entry to the maze And User char

        //If else statement to determine if user has selected fog or not.
        if (fog == 0) {
            //For loop to print maze without any fog
            for (int row = 0; row < maze.h; row++) {
                for (int col = 0; col < maze.w; col++) {
                    printf("%c", maze.a[row][col]);
                }
                printf("\n");
            }
        } else {
            //Implementing Fog
            int startRow = y - fog;
            int endRow = y + fog;
            int startCollum = x - fog;
            int endCollum = x + fog;
            //Fog coordinate out of bounds constraints
            if (startRow < 0) { startRow = 0; }
            if (startCollum < 0) { startCollum = 0; }
            if (endRow > maze.h - 1) { endRow = maze.h - 1; }
            if (endCollum > maze.w - 1) { endCollum = maze.w - 1;}
            //For loop to print maze with given fog variable by the user
            for (int i = startRow; i <= endRow; i++) {
                for (int j = startCollum; j <= endCollum; j++) {
                    printf("%c", maze.a[i][j]);
                }
                printf("\n");
            }
        }
        //After maze has been displayed return User char to ' ' bank field to remove old user char and also removes the potions once they have been picked since it turns all previous chars into ' ' blak's.
        maze.a[y][x] = ' ';

        //Printing User Coordinates,Amount of potions collected and ask User for an movement command(input)
        printf("X:%d Y:%d\n", x, y);
        printf("Potion:%d", potion);
        printf("\nMove(w,a,s,d):");
        scanf(" %c", &input);

        //Switch case to process movement command and validity
        switch (input) {
            case 'w':
                //Prevents user to go trough walls
                if (maze.a[y - 1][x] != 'w') {
                    y = y - 1;
                }
                break;
            case 's':
                if (maze.a[y + 1][x] != 'w') {
                    y = y + 1;
                }
                break;
            case 'a':
                //Prevent user to go trough walls and leaving the maze
                if (maze.a[y][x - 1] != 'w' && maze.a[y][x - 1] != maze.a[1][-1]) {
                    x = x - 1;
                }
                break;
            case 'd':
                //Prevent user to leave the maze unless they have collected all the potions
                if (y + 2 == maze.h && x == maze.w - 2 && potion != 3) {
                    printf("GET THE POTION\n");
                } else {
                    if (maze.a[y][x + 1] != 'w') {
                        x = x + 1;
                    }
                }
                break;
        }
    }

    //Congratulates the user for successfully leaving the maze.
    printf("\nCongratulations You Have Escaped The Maze\n");
    //Added a system pause so that the congratulations message is visible when using the .exe
    system("pause");
}




