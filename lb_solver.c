#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_WORD_LENGTH 50
#define MAX_WORDS 500000
#define MAX_LETTERS 26
#define MAX_GROUPS 10
#define MAX_GROUP_SIZE 10
#define MAX_SOLUTION_WORDS 100
#define INITIAL_STACK_CAPACITY 10
#define INITIAL_SOLUTIONS_CAPACITY 100

// Structure for a letter pair (for illegal transitions)
typedef struct {
    char first;
    char second;
} LetterPair;

// Structure for graph edge with associated word
typedef struct {
    char to_letter;
    char* word;
} Edge;

// Structure for graph node
typedef struct {
    char letter;
    Edge* edges;
    int edge_count;
    int edge_capacity;
} Node;

// Helper function to convert string to lowercase
void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Check if a character is in a set of characters
bool char_in_set(char c, char* set, int set_size) {
    for (int i = 0; i < set_size; i++) {
        if (set[i] == c) {
            return true;
        }
    }
    return false;
}

// Find which group a letter belongs to
int find_group_index(char letter, char groups[][MAX_GROUP_SIZE], int group_sizes[], int num_groups) {
    for (int i = 0; i < num_groups; i++) {
        for (int j = 0; j < group_sizes[i]; j++) {
            if (groups[i][j] == letter) {
                return i;
            }
        }
    }
    return -1;
}

// Check if a letter pair is illegal (same group)
bool is_illegal_pair(char first, char second, LetterPair* illegal_pairs, int illegal_pair_count) {
    for (int i = 0; i < illegal_pair_count; i++) {
        if (illegal_pairs[i].first == first && illegal_pairs[i].second == second) {
            return true;
        }
    }
    return false;
}

// Filter words that contain valid letter transitions
char** filter_valid_words(char* word_list[], int word_count, 
                          char groups[][MAX_GROUP_SIZE], int group_sizes[], int num_groups,
                          char all_letters[], int all_letter_count,
                          LetterPair* illegal_pairs, int illegal_pair_count,
                          int* valid_word_count) {
    
    char** valid_words = (char**)malloc(word_count * sizeof(char*));
    *valid_word_count = 0;
    
    for (int w = 0; w < word_count; w++) {
        char* word = word_list[w];
        bool valid = true;
        
        // Skip words containing letters not in our groups
        for (int i = 0; word[i]; i++) {
            if (!char_in_set(word[i], all_letters, all_letter_count)) {
                valid = false;
                break;
            }
        }
        
        if (!valid) continue;
        
        // Check for illegal transitions
        for (int i = 0; word[i+1]; i++) {
            if (is_illegal_pair(word[i], word[i+1], illegal_pairs, illegal_pair_count)) {
                valid = false;
                break;
            }
        }
        
        if (valid) {
            valid_words[*valid_word_count] = strdup(word);
            (*valid_word_count)++;
        }
    }
    
    return valid_words;
}

// Build a graph from valid words
void build_graph_from_words(char* valid_words[], int valid_word_count,
                           char all_letters[], int all_letter_count,
                           Node graph[], char** words_by_first_letter[]) {
    
    // Initialize graph
    for (int i = 0; i < all_letter_count; i++) {
        graph[i].letter = all_letters[i];
        graph[i].edge_count = 0;
        graph[i].edge_capacity = 10; // Initial capacity
        graph[i].edges = (Edge*)malloc(graph[i].edge_capacity * sizeof(Edge));
    }
    
    // Group words by first letter
    for (int i = 0; i < all_letter_count; i++) {
        words_by_first_letter[i] = (char**)malloc(valid_word_count * sizeof(char*));
        for (int j = 0; j < valid_word_count; j++) {
            words_by_first_letter[i][j] = NULL;
        }
    }
    
    int words_by_first_count[MAX_LETTERS] = {0};
    
    for (int w = 0; w < valid_word_count; w++) {
        char* word = valid_words[w];
        if (strlen(word) > 0) {
            char first = word[0];
            int first_idx = -1;
            
            // Find index of first letter in all_letters
            for (int i = 0; i < all_letter_count; i++) {
                if (all_letters[i] == first) {
                    first_idx = i;
                    break;
                }
            }
            
            if (first_idx != -1) {
                words_by_first_letter[first_idx][words_by_first_count[first_idx]] = word;
                words_by_first_count[first_idx]++;
            }
        }
    }
    
    // Build transitions in the graph
    for (int w = 0; w < valid_word_count; w++) {
        char* word = valid_words[w];
        int len = strlen(word);
        
        if (len > 0) {
            char last_letter = word[len - 1];
            int last_idx = -1;
            
            // Find index of last letter in all_letters
            for (int i = 0; i < all_letter_count; i++) {
                if (all_letters[i] == last_letter) {
                    last_idx = i;
                    break;
                }
            }
            
            if (last_idx != -1) {
                // Add edge for self-loop (can start a new word with this last letter)
                // Check if this edge already exists
                bool edge_exists = false;
                for (int e = 0; e < graph[last_idx].edge_count; e++) {
                    if (graph[last_idx].edges[e].to_letter == last_letter) {
                        edge_exists = true;
                        break;
                    }
                }
                
                if (!edge_exists) {
                    // Resize edges array if needed
                    if (graph[last_idx].edge_count >= graph[last_idx].edge_capacity) {
                        graph[last_idx].edge_capacity *= 2;
                        graph[last_idx].edges = (Edge*)realloc(graph[last_idx].edges, 
                                                             graph[last_idx].edge_capacity * sizeof(Edge));
                    }
                    
                    // Add self-loop edge
                    graph[last_idx].edges[graph[last_idx].edge_count].to_letter = last_letter;
                    graph[last_idx].edges[graph[last_idx].edge_count].word = NULL; // No specific word for self-loop
                    graph[last_idx].edge_count++;
                }
            }
        }
    }
}

// Calculate word score based on letter frequency
double word_score(char* word, int letter_frequency[]) {
    bool letter_seen[MAX_LETTERS] = {false};
    double score = 0.0;
    
    for (int i = 0; word[i]; i++) {
        char c = word[i];
        if (!letter_seen[c - 'a']) {
            letter_seen[c - 'a'] = true;
            score += 1.0 / (letter_frequency[c - 'a'] > 0 ? letter_frequency[c - 'a'] : 1);
        }
    }
    
    return score;
}

// Compare function for qsort used to sort words by score
int compare_words_by_score(const void* a, const void* b) {
    char* word1 = *(char**)a;
    char* word2 = *(char**)b;
    
    // This will be replaced with actual scoring logic when used
    return 0;
}

// Find a solution using greedy approach
char** find_path_greedy(char* valid_words[], int valid_word_count,
                        char all_letters[], int all_letter_count,
                        char groups[][MAX_GROUP_SIZE], int group_sizes[], int num_groups,
                        char** words_by_first_letter[], int* solution_size) {
    
    // Calculate letter frequencies
    int letter_frequency[MAX_LETTERS] = {0};
    
    for (int w = 0; w < valid_word_count; w++) {
        char* word = valid_words[w];
        bool seen[MAX_LETTERS] = {false};
        
        for (int i = 0; word[i]; i++) {
            int idx = word[i] - 'a';
            if (idx >= 0 && idx < MAX_LETTERS && !seen[idx]) {
                seen[idx] = true;
                letter_frequency[idx]++;
            }
        }
    }
    
    // Sort letters by frequency (rarest first)
    char rare_letters[MAX_LETTERS];
    int rare_letter_count = 0;
    
    for (int i = 0; i < all_letter_count; i++) {
        rare_letters[rare_letter_count++] = all_letters[i];
    }
    
    // Simple insertion sort for the rare letters
    for (int i = 1; i < rare_letter_count; i++) {
        char key = rare_letters[i];
        int idx_key = key - 'a';
        int j = i - 1;
        
        while (j >= 0 && letter_frequency[rare_letters[j] - 'a'] > letter_frequency[idx_key]) {
            rare_letters[j + 1] = rare_letters[j];
            j--;
        }
        rare_letters[j + 1] = key;
    }
    
    // Try starting with each rare letter
    for (int start_idx = 0; start_idx < rare_letter_count; start_idx++) {
        char start_letter = rare_letters[start_idx];
        int start_letter_idx = -1;
        
        // Find index of start letter in all_letters
        for (int i = 0; i < all_letter_count; i++) {
            if (all_letters[i] == start_letter) {
                start_letter_idx = i;
                break;
            }
        }
        
        if (start_letter_idx == -1) continue;
        
        // Get candidate words
        char** candidate_words = words_by_first_letter[start_letter_idx];
        int candidate_count = 0;
        
        // Count candidates
        while (candidate_words[candidate_count] != NULL && candidate_count < valid_word_count) {
            candidate_count++;
        }
        
        if (candidate_count == 0) continue;
        
        // Create a working copy for sorting
        char** sorted_candidates = (char**)malloc(candidate_count * sizeof(char*));
        for (int i = 0; i < candidate_count; i++) {
            sorted_candidates[i] = candidate_words[i];
        }
        
        // Sort candidates by word score (this would need custom compare function)
        // For simplicity, we'll use a straightforward approach instead of qsort
        for (int i = 0; i < candidate_count - 1; i++) {
            for (int j = i + 1; j < candidate_count; j++) {
                double score_i = word_score(sorted_candidates[i], letter_frequency);
                double score_j = word_score(sorted_candidates[j], letter_frequency);
                
                if (score_j > score_i) {
                    char* temp = sorted_candidates[i];
                    sorted_candidates[i] = sorted_candidates[j];
                    sorted_candidates[j] = temp;
                }
            }
        }
        
        // Try each starting word
        for (int first_word_idx = 0; first_word_idx < candidate_count; first_word_idx++) {
            char* first_word = sorted_candidates[first_word_idx];
            
            // Keep track of used letters
            bool used_letters[MAX_LETTERS] = {false};
            for (int i = 0; first_word[i]; i++) {
                int idx = first_word[i] - 'a';
                if (idx >= 0 && idx < MAX_LETTERS) {
                    used_letters[idx] = true;
                }
            }
            
            // Initialize solution
            char** solution = (char**)malloc(MAX_SOLUTION_WORDS * sizeof(char*));
            int solution_count = 0;
            solution[solution_count++] = strdup(first_word);
            
            char current_letter = first_word[strlen(first_word) - 1];
            
            // Count total letters to cover
            int total_letters_to_cover = 0;
            for (int i = 0; i < MAX_LETTERS; i++) {
                if (char_in_set(i + 'a', all_letters, all_letter_count)) {
                    total_letters_to_cover++;
                }
            }
            
            // Count covered letters
            int covered_letter_count = 0;
            for (int i = 0; i < MAX_LETTERS; i++) {
                if (used_letters[i] && char_in_set(i + 'a', all_letters, all_letter_count)) {
                    covered_letter_count++;
                }
            }
            
            // Keep adding words until we've covered all letters or can't proceed
            while (covered_letter_count < total_letters_to_cover) {
                // Find next word that covers the most new letters
                char* best_word = NULL;
                int best_score = 0;
                
                int current_letter_idx = -1;
                for (int i = 0; i < all_letter_count; i++) {
                    if (all_letters[i] == current_letter) {
                        current_letter_idx = i;
                        break;
                    }
                }
                
                if (current_letter_idx == -1) break;
                
                char** candidates = words_by_first_letter[current_letter_idx];
                int candidates_count = 0;
                
                // Count candidates
                while (candidates[candidates_count] != NULL && candidates_count < valid_word_count) {
                    candidates_count++;
                }
                
                // Check each candidate word
                for (int w = 0; w < candidates_count; w++) {
                    char* word = candidates[w];
                    bool valid = true;
                    
                    // Count new letters this word would add
                    int new_letters = 0;
                    for (int i = 0; word[i]; i++) {
                        int idx = word[i] - 'a';
                        if (idx >= 0 && idx < MAX_LETTERS && !used_letters[idx]) {
                            new_letters++;
                        }
                    }
                    
                    if (new_letters > best_score) {
                        best_score = new_letters;
                        best_word = word;
                    }
                }
                
                if (best_word == NULL || best_score == 0) {
                    break; // No word adds new letters
                }
                
                // Add best word to solution
                solution[solution_count++] = strdup(best_word);
                
                // Update used letters
                for (int i = 0; best_word[i]; i++) {
                    int idx = best_word[i] - 'a';
                    if (idx >= 0 && idx < MAX_LETTERS && !used_letters[idx]) {
                        used_letters[idx] = true;
                        if (char_in_set(idx + 'a', all_letters, all_letter_count)) {
                            covered_letter_count++;
                        }
                    }
                }
                
                // Update current letter
                current_letter = best_word[strlen(best_word) - 1];
            }
            
            // Check if we covered all letters
            if (covered_letter_count == total_letters_to_cover) {
                free(sorted_candidates);
                *solution_size = solution_count;
                return solution;
            }
            
            // Free solution memory if not successful
            for (int i = 0; i < solution_count; i++) {
                free(solution[i]);
            }
            free(solution);
        }
        
        free(sorted_candidates);
    }
    
    *solution_size = 0;
    return NULL;
}

bool covers_all_letters(bool used_letters[MAX_LETTERS], char all_letters[], int all_letter_count) {
    for (int i = 0; i < all_letter_count; i++) {
        if (!used_letters[all_letters[i] - 'a'])
            return false;
    }
    return true;
}

// Stack frame structure for DFS.
typedef struct {
    int depth;              // Number of words chosen so far.
    int candidate_index;    // Next candidate index to try in candidate_list.
    int candidate_count;    // Total candidates in candidate_list.
    char** candidate_list;  // Candidate list for this level (for depth 0: valid_words; for depth > 0: words_by_first_letter for current letter).
    bool used_letters[MAX_LETTERS]; // Letters used up to this state.
    char current_letter;    // The letter that determined the candidate list (if depth > 0).
    char* word;             // The word chosen at this frame (NULL for initial frame).
} Frame;

// Helper: Print current solution from stack frames (ignoring the initial frame).
void print_solution(Frame stack[], int top) {
    printf("Found solution (%d words): ", top - 1);
    for (int i = 1; i < top; i++) {
        printf("%s ", stack[i].word);
    }
    printf("\n");
}

bool is_word_used(char* candidate, char* current_solution[], int solution_count) {
    for (int i = 0; i < solution_count; i++) {
        if (strcmp(current_solution[i], candidate) == 0)
            return true;
    }
    return false;
}

// Iterative exhaustive search using an explicit DFS stack.
void find_paths_exhaustive(char* valid_words[], int valid_word_count,
                                     char all_letters[], int all_letter_count,
                                     char groups[][MAX_GROUP_SIZE], int group_sizes[], int num_groups,
                                     char** words_by_first_letter[]) {
    int best_solution_count = 5;  // No solution found yet; use a very high initial value.
    unsigned long long iteration_count = 0;
    
    // Allocate a sufficiently large stack.
    Frame stack[1000];
    int stack_top = 0;
    
    // Initialize the initial frame (depth 0): candidate list is all valid words.
    memset(&stack[stack_top], 0, sizeof(Frame));
    stack[stack_top].depth = 0;
    stack[stack_top].candidate_index = 0;
    stack[stack_top].candidate_list = valid_words;
    stack[stack_top].candidate_count = valid_word_count;
    // used_letters remains all false; word remains NULL.
    stack_top++;
    
    // Iterative DFS.
    while (stack_top > 0) {
        iteration_count++;
        if (iteration_count % 10000 == 0) {
            printf("Solutions Searched: %llu\r", iteration_count);
            fflush(stdout);
        }
        
        Frame *current = &stack[stack_top - 1];
        // Prune this branch if adding another word would equal/exceed the current best solution.
        if (current->depth >= best_solution_count - 1) {
            stack_top--;  // Backtrack.
            continue;
        }
        
        if (current->candidate_index < current->candidate_count) {
            // Get the next candidate word.
            char* candidate = current->candidate_list[current->candidate_index];
            current->candidate_index++;

            // Check if the candidate word is already used in the current solution.
            if (is_word_used(candidate, stack[0].candidate_list, current->depth)) {
                continue;
            }
            
            // Prepare a new frame for the candidate.
            Frame new_frame;
            new_frame.depth = current->depth + 1;
            new_frame.word = candidate;
            // Copy parent's used_letters and update with letters from candidate.
            memcpy(new_frame.used_letters, current->used_letters, sizeof(bool) * MAX_LETTERS);
            for (int i = 0; candidate[i] != '\0'; i++) {
                new_frame.used_letters[candidate[i] - 'a'] = true;
            }
            // New current letter is the last letter of the candidate.
            new_frame.current_letter = candidate[strlen(candidate) - 1];
            
            // Set candidate list for new frame using words_by_first_letter.
            int letter_index = -1;
            for (int i = 0; i < all_letter_count; i++) {
                if (all_letters[i] == new_frame.current_letter) {
                    letter_index = i;
                    break;
                }
            }
            if (letter_index == -1) {
                // No candidate list available; skip this branch.
                continue;
            }
            new_frame.candidate_list = words_by_first_letter[letter_index];
            // Count the number of candidates in the list.
            int count = 0;
            while (new_frame.candidate_list[count] != NULL && count < valid_word_count)
                count++;
            new_frame.candidate_count = count;
            new_frame.candidate_index = 0;
            
            // Push the new frame onto the stack.
            stack[stack_top] = new_frame;
            stack_top++;
            
            // Check if the current chain covers all required letters.
            if (covers_all_letters(new_frame.used_letters, all_letters, all_letter_count)) {
                if (new_frame.depth < best_solution_count) {
                    best_solution_count = new_frame.depth;
                    print_solution(stack, stack_top);
                }
                // Backtrack from this branch after finding a solution.
                stack_top--;
            }
        } else {
            // All candidates for this frame have been tried; backtrack.
            stack_top--;
        }
    }
    
    // Final progress update.
    printf("Search complete. Total candidate solutions searched: %llu\n", iteration_count);
}

// Function to safely ask the user for 12 letters in four groups of three
void get_user_input_groups(char groups[MAX_GROUPS][MAX_GROUP_SIZE], int group_sizes[], int num_groups) {
    printf("Please enter 12 letters in four groups of three. Use [ * * * ] [ * * * ] ... format.\n");
    printf("Type 'del' to delete the previous letter in case of a mistake.\n");

    int total_letters = 0;
    char input[10]; // Buffer for user input
    memset(groups, 0, sizeof(char) * MAX_GROUPS * MAX_GROUP_SIZE);

    while (total_letters < 12) {
        // Move cursor to the top line and clear it
        printf("\033[F\033[K"); // Move cursor up one line and clear the line
        printf("[ ");
        for (int g = 0; g < num_groups; g++) {
            for (int i = 0; i < group_sizes[g]; i++) {
                if (groups[g][i] != '\0') {
                    printf("%c ", groups[g][i]);
                } else {
                    printf("* ");
                }
            }
            if (g < num_groups - 1) {
                printf("] [ ");
            }
        }
        printf("]\n");

        // Prompt user for input on the second line
        printf("Enter a letter or 'del': ");
        fflush(stdout);

        // Get user input
        scanf("%s", input);

        if (strcmp(input, "del") == 0) {
            // Delete the last entered letter
            if (total_letters > 0) {
                total_letters--;
                int group = total_letters / 3;
                int index = total_letters % 3;
                groups[group][index] = '\0';
            }
        } else if (strlen(input) == 1 && isalpha(input[0])) {
            // Add the letter to the next available slot
            char letter = tolower(input[0]);
            int group = total_letters / 3;
            int index = total_letters % 3;
            groups[group][index] = letter;
            total_letters++;
        } else {
            printf("\033[F\033[KInvalid input. Please enter a single letter or 'del'.\n");
        }
        printf("\033[F\033[K"); // Move cursor up one line and clear the line
    }

    // Final display of input
    printf("\033[F\033[K[ ");
    for (int g = 0; g < num_groups; g++) {
        for (int i = 0; i < group_sizes[g]; i++) {
            printf("%c ", groups[g][i]);
        }
        if (g < num_groups - 1) {
            printf("] [ ");
        }
    }
    printf("]\n");
}

// Load a word list from file
char** load_word_list(const char* file_path, int* word_count) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        *word_count = 0;
        return NULL;
    }
    
    char** words = (char**)malloc(MAX_WORDS * sizeof(char*));
    *word_count = 0;
    char buffer[MAX_WORD_LENGTH];
    
    while (fgets(buffer, MAX_WORD_LENGTH, file) && *word_count < MAX_WORDS) {
        // Remove newline
        size_t len = strlen(buffer);
        if (len > 0 && (buffer[len-1] == '\n')) {
            buffer[len-1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len-1] == '\r') {
            buffer[--len] = '\0';
        }
        
        if (len > 0) {
            to_lowercase(buffer);
            words[*word_count] = strdup(buffer);
            (*word_count)++;
        }
    }
    
    fclose(file);

    return words;
}

// Get a small set of common English words for testing
char** get_common_english_words(int* word_count) {
    const char* common_words[] = {
        "the", "be", "to", "of", "and", "a", "in", "that", "have", "it", "for", "not", "on", "with", 
        "he", "as", "you", "do", "at", "this", "but", "his", "by", "from", "they", "we", "say", "her", 
        "she", "or", "an", "will", "my", "one", "all", "would", "there", "their", "what", "so", "up", 
        "out", "if", "about", "who", "get", "which", "go", "me", "when", "make", "can", "like", "time", 
        "no", "just", "him", "know", "take", "people", "into", "year", "your", "good", "some", "could", 
        "them", "see", "other", "than", "then", "now", "look", "only", "come", "its", "over", "think", 
        "also", "back", "after", "use", "two", "how", "our", "work", "first", "well", "way", "even", 
        "new", "want", "because", "any", "these", "give", "day", "most", "us"
    };
    
    int common_count = sizeof(common_words) / sizeof(common_words[0]);
    char** words = (char**)malloc(common_count * sizeof(char*));
    
    for (int i = 0; i < common_count; i++) {
        words[i] = strdup(common_words[i]);
    }
    
    *word_count = common_count;
    return words;
}

int main() {
    // Example with 4 groups of 3 letters each
    char example_groups[MAX_GROUPS][MAX_GROUP_SIZE] = {
        {'l', 'u', 'v'},
        {'q', 'r', 'w'},
        {'m', 'e', 'o'},
        {'s', 'y', 'i'}
    };
    
    char letter_groups[MAX_GROUPS][MAX_GROUP_SIZE] = {0};
    int group_sizes[MAX_GROUPS] = {3, 3, 3, 3};
    int num_groups = 4;

    do {
        get_user_input_groups(letter_groups, group_sizes, num_groups);
        
        // Flatten all letters and track groups
        char all_letters[MAX_LETTERS];
        int all_letter_count = 0;
        
        for (int g = 0; g < num_groups; g++) {
            for (int i = 0; i < group_sizes[g]; i++) {
                all_letters[all_letter_count++] = letter_groups[g][i];
            }
        }
        
        // Create illegal pairs (letters from same group)
        LetterPair illegal_pairs[MAX_LETTERS * MAX_LETTERS];
        int illegal_pair_count = 0;
        
        for (int g = 0; g < num_groups; g++) {
            for (int i = 0; i < group_sizes[g]; i++) {
                for (int j = 0; j < group_sizes[g]; j++) {
                    illegal_pairs[illegal_pair_count].first = letter_groups[g][i];
                    illegal_pairs[illegal_pair_count].second = letter_groups[g][j];
                    illegal_pair_count++;
                }
            }
        }

        // Load word list
        int word_count;
        char** words;
        
        if (access("2of12.txt", F_OK) != -1) {
            words = load_word_list("2of12.txt", &word_count);
            printf("Loaded %d words from file\n", word_count);
        } else {
            printf("Using built-in word list (limited)\n");
            words = get_common_english_words(&word_count);
            printf("Loaded %d words into dictionary\n", word_count);
        }

        // Filter valid words
        int valid_word_count;
        char** valid_words = filter_valid_words(words, word_count, 
                                              letter_groups, group_sizes, num_groups,
                                              all_letters, all_letter_count,
                                              illegal_pairs, illegal_pair_count,
                                              &valid_word_count);
        
        printf("Filtered dictionary from %d to %d valid words\n", word_count, valid_word_count);
        
        // Build graph
        Node graph[MAX_LETTERS];
        char** words_by_first_letter[MAX_LETTERS];
        
        build_graph_from_words(valid_words, valid_word_count,
                             all_letters, all_letter_count,
                             graph, words_by_first_letter);

        // Find solution

        find_paths_exhaustive(valid_words, valid_word_count,
            all_letters, all_letter_count,
            letter_groups, group_sizes, num_groups,
            words_by_first_letter);
        
        // Free memory
        for (int i = 0; i < all_letter_count; i++) {
            free(graph[i].edges);
            free(words_by_first_letter[i]);
        }
        
        for (int i = 0; i < valid_word_count; i++) {
            free(valid_words[i]);
        }
        free(valid_words);
        
        for (int i = 0; i < word_count; i++) {
            free(words[i]);
        }
        free(words);

        printf("Would you like to try again? [y/n]: ");
        char response;
        scanf(" %c", &response);
        if (tolower(response) != 'y') {
            break;
        }
    } while (1);

    return 0;
}