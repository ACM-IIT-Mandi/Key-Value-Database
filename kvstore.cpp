// A basic key-value database in C++17.
// Features: SET, GET, DEL, EXISTS, KEYS, COUNT, CLEAR, persistence to a file.
//
// Build:  g++ -std=c++17 -O2 -o kvstore kvstore.cpp
// Run:    ./kvstore [datafile]        (default datafile: data.db)

#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>

class KVStore
{
public:
	explicit KVStore(std::string path) : path_(std::move(path)) { load(); }

	void set(const std::string &key, const std::string &value)
	{
		data_[key] = value;
		save();
	}

	std::optional<std::string> get(const std::string &key) const
	{
		auto it = data_.find(key);
		if (it == data_.end())
			return std::nullopt;
		return it->second;
	}

	bool del(const std::string &key)
	{
		bool removed = data_.erase(key) > 0;
		if (removed)
			save();
		return removed;
	}

	bool exists(const std::string &key) const { return data_.count(key) > 0; }
	size_t count() const { return data_.size(); }

	void clear()
	{
		data_.clear();
		save();
	}

	const std::map<std::string, std::string> &all() const { return data_; }

private:
	// File format: one record per line, "<keylen> <valuelen>\n<key><value>\n"
	// so keys and values may contain spaces and newlines.
	void save() const
	{
		std::ofstream out(path_ + ".tmp", std::ios::binary | std::ios::trunc);
		for (const auto &[k, v] : data_)
			out << k.size() << ' ' << v.size() << '\n'
				<< k << v << '\n';
		out.close();
		std::rename((path_ + ".tmp").c_str(), path_.c_str()); // replace atomically
	}

	void load()
	{
		std::ifstream in(path_, std::ios::binary);
		size_t klen, vlen;
		while (in >> klen >> vlen)
		{
			in.get(); // newline
			std::string k(klen, '\0'), v(vlen, '\0');
			in.read(&k[0], klen);
			in.read(&v[0], vlen);
			in.get(); // newline
			if (!in)
				break;
			data_[k] = v;
		}
	}

	std::string path_;
	std::map<std::string, std::string> data_;
};

int main(int argc, char **argv)
{
	KVStore db(argc > 1 ? argv[1] : "data.db");
	std::cout << "Simple KV store. Commands: SET key value | GET key | DEL key | "
				 "EXISTS key | KEYS | COUNT | CLEAR | QUIT\n";

	std::string line;
	while (std::cout << "> " && std::getline(std::cin, line))
	{
		std::istringstream ss(line);
		std::string cmd, key;
		ss >> cmd >> key;
		for (auto &c : cmd)
			c = toupper(c);

		if (cmd == "SET")
		{
			std::string value;
			std::getline(ss >> std::ws, value);
			if (key.empty() || value.empty())
			{
				std::cout << "usage: SET key value\n";
				continue;
			}
			db.set(key, value);
			std::cout << "OK\n";
		}
		else if (cmd == "GET")
		{
			auto v = db.get(key);
			std::cout << (v ? *v : "(not found)") << "\n";
		}
		else if (cmd == "DEL")
		{
			std::cout << (db.del(key) ? "deleted" : "(not found)") << "\n";
		}
		else if (cmd == "EXISTS")
		{
			std::cout << (db.exists(key) ? "yes" : "no") << "\n";
		}
		else if (cmd == "KEYS")
		{
			for (const auto &[k, v] : db.all())
				std::cout << k << "\n";
		}
		else if (cmd == "COUNT")
		{
			std::cout << db.count() << "\n";
		}
		else if (cmd == "CLEAR")
		{
			db.clear();
			std::cout << "OK\n";
		}
		else if (cmd == "QUIT" || cmd == "EXIT")
		{
			break;
		}
		else if (!cmd.empty())
		{
			std::cout << "unknown command\n";
		}
	}
	return 0;
}
