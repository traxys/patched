/**
 * @file
 * \author Quentin Boyer
 */
#include <iostream>
#include <limits.h>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <iomanip>
#include <fstream>
#define INF INT_MAX

class Lines {
	public:
		Lines(const char* file_name);
		std::string_view get(int64_t i);
		inline int line_count() {
			return this->line_end.size();
		}
		inline int length(int64_t i) {
			if(i >= line_count()) {
				return -1;
			}
			int64_t start = 0;
			if (i != 0) {
				start = this->line_end[i - 1] + 1;
			}
			return this->line_end[i] - start;
		}
	private:
		/**
		 * \brief Delimit the next file from the buffer
		 * \return false if there are no more lines
		 */
		bool delim_next_line();

		int fd;
		char* buffer;
		size_t size;
		std::vector<int64_t> line_end;
		size_t remaining;
};
Lines::Lines(const char* file_name) {
	this->fd = open(file_name, O_RDONLY | O_LARGEFILE);
	struct stat sb;
	if (fstat(fd, &sb) < 0) {
		std::cerr << "Error stating file" << std::endl;
	}
	this->size = sb.st_size;
	this->remaining = sb.st_size;

	this->buffer = (char*) mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (this->buffer == MAP_FAILED) {
		std::cerr << "Failed to mmap file" << std::endl;
	}
	this->line_end = std::vector<int64_t>();
	while(this->delim_next_line()) {}
}
std::string_view Lines::get(int64_t i) {
	if(i >= this->line_count()) {
		return std::string_view("<NO SUCH LINE IN FILE>");
	}
	int64_t start = 0;
	if (i != 0) {
		start = this->line_end[i - 1] + 1;
	}
	return std::string_view(this->buffer + start, this->line_end[i] - start);
}
bool Lines::delim_next_line() {
	int previous_end;
	if (this->line_end.empty()) {
		previous_end = -1;
	} else {
		previous_end = this->line_end.back();
	}
	int offset = 1;
	if(remaining == 0) {
		return false;
	}
	while(remaining > offset && buffer[previous_end + offset] != '\n') {
		offset += 1;
	}
	remaining -= offset;
	this->line_end.push_back(previous_end + offset);
	return true;
}

struct hash_pair final {
	template<class TFirst, class TSecond>
		size_t operator()(const std::pair<TFirst, TSecond>& p) const noexcept {
			uintmax_t hash = std::hash<TFirst>{}(p.first);
			hash <<= sizeof(uintmax_t) * 4;
			hash ^= std::hash<TSecond>{}(p.second);
			return std::hash<uintmax_t>{}(hash);
		}
};
using LinePair = std::pair<int64_t, int64_t>;

struct Path {
	int64_t total_cost;
	LinePair end_point;

	Path(int64_t initial_cost, LinePair start_point);
};
Path::Path(int64_t initial_cost,LinePair start_point) {
	this->total_cost = initial_cost;
	this->end_point = start_point;
}
bool is_cheaper(const Path& a, const Path& b) {
	return a.total_cost > b.total_cost;
}

struct Cost {
	int64_t cost;
	LinePair previous;
};
class Searcher {
	public:
		Searcher(Lines* input, Lines* output);
		void search();
		std::vector<LinePair> optimal_path();
		void print_graph();
		int64_t optimal_cost();
	private:
		// If true, means that one of the ends stopped on the end point
		bool advance_min_path();
		void min_path_to(Path& min_path, int64_t incr_cost, LinePair next_point);
		Lines* a;
		Lines* b;
		std::unordered_map<LinePair, Cost, hash_pair> costs;
		std::vector<Path> paths;
};
Searcher::Searcher(Lines* input, Lines* output) {
	this->a = input;
	this->b = output;
	this->paths = std::vector<Path>();
	this->costs = std::unordered_map<LinePair, Cost, hash_pair>();
	for(int i = -1; i < this->a->line_count(); ++i) {
		LinePair p = {i, -1};
		int64_t cost = 10 * (i + 1);
		auto pth = Path(cost, p);
		this->costs.insert({p, {cost, {-1, -1}}});
		this->paths.push_back(pth);
	}
	int64_t sum = 0;
	for(int j = 0; j < this->b->line_count(); ++j) {
		sum += this->b->length(j) + 10;
		LinePair p = {-1, j};
		auto pth = Path(sum, p);
		this->costs.insert({p, {sum, {-1, -1}}});
		this->paths.push_back(pth);
	}
	std::make_heap(this->paths.begin(), this->paths.end(), is_cheaper);
}
void Searcher::search() {
	while(!this->advance_min_path()) {}
}
bool Searcher::advance_min_path() {
	std::pop_heap(this->paths.begin(), this->paths.end(), is_cheaper);
	Path min_path = this->paths.back();
	this->paths.pop_back();

	auto i = min_path.end_point.first;
	auto j = min_path.end_point.second;
		/*std::cout 	<< "[MIN_PATH] i: " << i 
					<< " | j: " << j
					<< std::endl;*/
	if ( i == this->a->line_count() - 1 && j == this->b->line_count() - 1) {
		// We have a min path that is at the end, we finished
		return true;
	}
	bool need_remake_heap = false;
	// We can read check for one more line of A, more remain, if we are not at a border
	if (j != -1 && i < this->a->line_count() - 1) {
		auto incr_i_cost = 10;
		LinePair next_point = {i + 1, j};
		this->min_path_to(min_path, incr_i_cost, next_point);
	}
	if (i != -1 && j < this->b->line_count() - 1) {
		auto incr_j_cost = 10 + this->b->length(j + 1);
		LinePair next_point = {i, j + 1};
		this->min_path_to(min_path, incr_j_cost, next_point);
	}
	if (j < this->b->line_count() - 1 && i < this->a->line_count() - 1) {
		int64_t incr_ij_cost;
		if (this->a->get(i + 1) == this->b->get(j + 1)) { // We should hopefully only check once per segment, so it should be good
			incr_ij_cost = 0;
		} else {
			incr_ij_cost = 10 + this->b->length(j + 1);
		}
		LinePair next_point = {i + 1, j + 1};
		this->min_path_to(min_path, incr_ij_cost, next_point);
	}
	if(need_remake_heap) {
		std::make_heap(this->paths.begin(), this->paths.end(), is_cheaper);
	}
	return false;
}
void Searcher::min_path_to(Path& min_path, int64_t incr_cost, LinePair next_point) {
	// If we are on a path that was already discovered there is a shorter
	// path to go to that point, so we can delete don't have to add ourselves
	if (!this->costs.contains(next_point)) {
		auto new_path = Path(min_path.total_cost + incr_cost,next_point);
		// We are at a new point we can update ourselves
		auto prev_point = min_path.end_point;
		/*std::cout 	<< "[COST] i: " << next_point.first 
					<< " | j: " << next_point.second 
					<< " costed at:" << new_path->total_cost 
					<< std::endl;*/
		this->costs.insert({new_path.end_point, {new_path.total_cost, prev_point}});
		this->paths.push_back(new_path);
		std::push_heap(this->paths.begin(), this->paths.end(), is_cheaper);
	}
}
std::vector<LinePair> Searcher::optimal_path() {
	auto opt_path = std::vector<LinePair>();
	opt_path.push_back({
		this->a->line_count() - 1,
		this->b->line_count() - 1
	});
	auto last_seen = opt_path.back();
	while(last_seen.first >= 0 && last_seen.second >= 0) {
		opt_path.push_back(this->costs.at(last_seen).previous);
		last_seen = opt_path.back();
	}
	std::reverse(opt_path.begin(), opt_path.end());
	return opt_path;
}
void Searcher::print_graph() {
	for(int j = 0; j < this->b->line_count(); j++) {
		for(int i = 0; i < this->a->line_count(); i++) {
			LinePair p = {i, j};
			if(this->costs.contains(p)) {
				std::cout << std::setw(2) << this->costs[p].previous.first
						  << "," 
						  << std::setw(2) << this->costs[p].previous.second 
						  << " | ";
			} else {
				std::cout << std::setw(2) << "*" << "," << std::setw(2) << "*" << " | ";
			}
		}
		std::cout << std::endl;
	}
}
int64_t Searcher::optimal_cost() {
	return this->costs.at({
		this->a->line_count() - 1,
		this->b->line_count() - 1
	}).cost;
}
enum ACTION {
	PLUS = 1,
	MINUS = 2,
	SUBST = 3,
};
// TO CALL IF optimal_path.size() > 1
void create_patch(std::vector<LinePair>& optimal_path, const char* file_name, Lines* input, Lines* output) {
	std::ofstream ostrm(file_name);
	LinePair start = optimal_path[0];
	if(start.first == -1 && start.second == -1) {
		// Nothing to preset
	} else if (start.first == -1) {
		for(int j = 0; j < start.second; j++) {
			ostrm << "+ " << j + 1 << std::endl << output->get(j) << std::endl;
		}
	} else if (start.second == -1) {
		for(int i = 0; i < start.second; i++) {
			ostrm << "- " << i + 1 << std::endl << input->get(i) << std::endl;
		}
	}
	for(int64_t k = 1; k < optimal_path.size(); k++) {
		int action = 0;
		if(optimal_path[k - 1].first != optimal_path[k].first) {
			// i incremented, so it is a leat a MINUS
			action |= MINUS;
		}
		if(optimal_path[k - 1].second != optimal_path[k].second) {
			// j incremented, so it is at least a PLUS
			action |= PLUS;
		}
		ACTION act = (ACTION)action;
		switch (act) {
			case PLUS:
				ostrm << "+ " << optimal_path[k].first + 1 << std::endl << output->get(optimal_path[k].second) << std::endl;
				break;
			case MINUS:
				ostrm << "- " << optimal_path[k].first + 1 << std::endl << input->get(optimal_path[k].first) << std::endl;
				break;
			case SUBST:
				auto input_line = input->get(optimal_path[k].first);
				auto output_line = output->get(optimal_path[k].second);
				if (input_line != output_line) {
					ostrm << "= " << optimal_path[k].first + 1 << std::endl << input_line << std::endl << output_line << std::endl;
				}
				break;
		}
	}
}



/**
 * \fn int main (void)
 * \brief Entrée du programme.
 *
 * \return EXIT_SUCCESS - Arrêt normal du programme.
 */
int main(int argc, char* argv[]) {
	auto shortest_path = std::vector<Path>();
	if (argc != 4) {
		std::cerr << "Invalid argument count: expected 3 got " << argc - 1 << std::endl;
		return 1;
	}
	auto input_file = Lines(argv[1]);
	auto output_file = Lines(argv[2]);
	auto searcher = Searcher(&input_file, &output_file);
	//searcher.print_graph();
	searcher.search();
	auto optimal_path = searcher.optimal_path();
	for(auto& p: optimal_path) {
		std::cout << "i: " << p.first << " | j: " << p.second <<std::endl;
	}
	std::cout << "Valued at " << searcher.optimal_cost() << std::endl;
	create_patch(optimal_path, argv[3], &input_file, &output_file);


	return 0;
}
