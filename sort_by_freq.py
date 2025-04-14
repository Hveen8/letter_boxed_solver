from wordfreq import word_frequency

def sort_by_general_frequency(input_file, output_file):
    
    # Read all words from the input file
    with open(input_file, 'r') as file:
        words = [line.strip() for line in file]
    
    # Sort words by their frequency in English (descending)
    sorted_words = sorted(words, key=lambda word: word_frequency(word, 'en'), reverse=True)
    
    # Write sorted words to output file
    with open(output_file, 'w') as file:
        for word in sorted_words:
            file.write(f"{word}\n")
    
    print(f"Sorted {len(sorted_words)} words by frequency in English language.")

if __name__ == "__main__":

    input_file = "2of12inf.txt"
    output_file = "2of12inf_sorted.txt"  # Output file name

    sort_by_general_frequency(input_file, output_file)