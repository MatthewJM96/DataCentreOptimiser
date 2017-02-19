#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

// What lies below is an utter mess at this point. It needs cleaning up and optimising, but it's for the Google Hashcode comp, and ain't nobody got time to do that in 4 hours.


struct Server {
    int id;
    int poolID;
    int capacity;
    int size;
    float power; // capacity / size
};

struct Row {
    std::vector<int> unavailableSlots;
    std::vector<std::pair<int, Server>> servers;
    std::vector<std::pair<int, int>> availableSlots;
    int capacity;
    int slotUsage;
};

// Warning: can add same server multiple times, don't be stupid.
void add(Row& row, const Server& server, int firstSlot) {
    row.servers.push_back({ firstSlot, server });
    row.capacity += server.capacity;
    row.slotUsage += server.size;

    for (auto& it = row.availableSlots.begin(); it != row.availableSlots.end(); ++it) {
        if (firstSlot >= (*it).first && firstSlot < (*it).first + (*it).second) {
            auto temp = (*it);
            row.availableSlots.erase(it);

            if (firstSlot != temp.first) {
                row.availableSlots.push_back({ temp.first, firstSlot - temp.first });
            }

            if (firstSlot + server.size != temp.first + temp.second) {
                row.availableSlots.push_back({ firstSlot + server.size, temp.first + temp.second - (firstSlot + server.size) });
            }
            break;
        }
    }
}
void remove(Row& row, const Server& server) {
    for (auto& it = row.servers.begin(); it != row.servers.end(); ++it) {
        if ((*it).second.id == server.id) {
            row.servers.erase(it);
        }
    }
    row.capacity -= server.capacity;
    row.slotUsage -= server.size;
}


struct Pool {
    int id;
    std::vector<Server> servers;
    int capacity;
};

// Warning: can add same server multiple times, don't be stupid.
void add(Pool& pool, const Server& server) {
    pool.servers.push_back(server);
    pool.capacity += server.capacity;
}
void remove(Pool& pool, const Server& server) {
    for (auto& it = pool.servers.begin(); it != pool.servers.end(); ++it) {
        if ((*it).id == server.id) {
            pool.servers.erase(it);
        }
    }
    pool.capacity -= server.capacity;
}

struct DataCentre {
    int rowCount;
    int rowSize;
    std::vector<Row> rows;
    int poolCount;
    std::vector<Pool> pools;
    int serverCount;
    std::vector<Server> servers;
};

/// Model that holds the data centre data.
class DataCentreLoader {
public:
    DataCentreLoader() : m_dataCentre({}) {}
    ~DataCentreLoader() {
        dispose();
    }

    void init(std::string filepath = "medium.in") {
        m_filepath = filepath;
    }
    void dispose() {
        // Close file is still open.
        if (m_file.is_open()) {
            m_file.close();
        }

        // Clear up pizza.
        m_dataCentre = {};
    }

    DataCentre getDataCentre() {
        // If we haven't yet loaded data from the file, do so.
        if (m_dataCentre.rowCount == 0) {
            loadDataFromFile();
        }
        return m_dataCentre;
    }
private:
    bool openFile() {
        m_file.open(m_filepath, std::ios::in);
        return m_file.is_open();
    }

    void initialiseRows() {
        for (int i = 0; i < m_dataCentre.rowCount; ++i) {
            m_dataCentre.rows[i].availableSlots.push_back({ 0, m_dataCentre.rowSize });
        }

        for (Row& r : m_dataCentre.rows) {
            for (int i : r.unavailableSlots) {
                for (auto& it = r.availableSlots.begin(); it != r.availableSlots.end(); ++it) {
                    if (i >= (*it).first && i < (*it).first + (*it).second) {
                        auto temp = (*it);
                        r.availableSlots.erase(it);

                        if (i != temp.first) {
                            r.availableSlots.push_back({ temp.first, i - temp.first });
                        }

                        if (i + 1 != temp.first + temp.second) {
                            r.availableSlots.push_back({ i + 1, temp.first + temp.second - (i + 1) });
                        }
                        break;
                    }
                }
            }
        }
    }

    void initialisePools() {
        for (int i = 0; i < m_dataCentre.poolCount; ++i) {
            m_dataCentre.pools[i] = {};
            m_dataCentre.pools[i].id = i;
        }
    }

    void loadDataFromFile() {
        // If file isn't already open, and failed to open on an attempt, exit the program.
        if (!m_file.is_open() && !openFile()) {
            std::cout << "Could not open file: " << m_filepath << "." << std::endl
                << "Exiting..." << std::endl;
            std::getchar();
            exit(0);
        }

        // Grab metadata.
        std::string metadata;
        std::getline(m_file, metadata);

        std::stringstream stream(metadata);

        stream >> m_dataCentre.rowCount;
        m_dataCentre.rows.resize(m_dataCentre.rowCount);
        stream >> m_dataCentre.rowSize;

        int unavailableSlotCount = 0;
        stream >> unavailableSlotCount;

        stream >> m_dataCentre.poolCount;
        m_dataCentre.pools.resize(m_dataCentre.poolCount);
        initialisePools();

        stream >> m_dataCentre.serverCount;
        m_dataCentre.servers.resize(m_dataCentre.serverCount);

        std::string line;
        for (int i = 0; i < unavailableSlotCount; ++i) {
            std::getline(m_file, line);
            std::stringstream sstream(line);

            int rowInd, slotInd;
            sstream >> rowInd;
            sstream >> slotInd;

            m_dataCentre.rows[rowInd].unavailableSlots.push_back(slotInd);
        }

        initialiseRows();

        for (int i = 0; i < m_dataCentre.serverCount; ++i) {
            Server server = {};

            std::getline(m_file, line);
            std::stringstream sstream(line);

            int size, capacity;
            sstream >> size;
            sstream >> capacity;

            float power = (float)capacity / (float)size;

            server.id = i;
            server.capacity = capacity;
            server.size = size;
            server.power = power;

            m_dataCentre.servers[i] = server;
        }
    }

    std::fstream m_file;
    std::string m_filepath;

    DataCentre m_dataCentre;
};

class IdealSolver {
public:
    void init(DataCentre dc) {
        m_dataCentre = dc;
    }

    void solve() {
        //std::vector<Server> servers = m_dataCentre.servers;
        //
        //std::sort(servers.begin(), servers.end(), [](Server a, Server b) {
        //    return a.capacity > b.capacity;
        //});

        //std::vector<Pool> pools = m_dataCentre.pools;

        //for (Server s : servers) {
        //    int poolCapacity = INT_MAX;
        //    int poolID = -1;

        //    for (Pool p : pools) {
        //        if (p.capacity < poolCapacity) {
        //            poolID = p.id;
        //            poolCapacity = p.capacity;
        //        }
        //    }

        //    auto& it = std::find_if(pools.begin(), pools.end(), [&poolID](Pool a) {
        //        return a.id == poolID;
        //    });
        //    add((*it), s);
        //}

        //for (Pool p : pools) {
        //    std::cout << p.capacity << std::endl;
        //}

        std::vector<Server> servers = m_dataCentre.servers;
        
        std::sort(servers.begin(), servers.end(), [](Server a, Server b) {
            return a.capacity > b.capacity;
        });

        std::vector<Row> rows = m_dataCentre.rows;

        for (Server s : servers) {
            int rowCapacity = INT_MAX;
            int rowID = -1;

            for (int i = 0; i < rows.size(); ++i) {
                Row r = rows[i];
                if (r.capacity < rowCapacity) {
                    rowID = i;
                    rowCapacity = r.capacity;
                }
            }

            add(rows[rowID], s, 0);
        }
        
        m_result = rows;
    }

    std::vector<Row> getResult() {
        return m_result;
    }
private:
    std::vector<Row> m_result;

    DataCentre m_dataCentre;
};

class BinSolver {
public:
    void init(DataCentre dc, std::vector<Row> idealRows) {
        m_dataCentre = dc;
        m_idealRows = idealRows;
    }

    void solve() {
        std::vector<std::vector<Server>> availableServers;

        for (int i = 0; i < m_idealRows.size(); ++i) {
            Row r = m_idealRows[i];
            std::vector<Server> rowServers;
            for (auto& s : r.servers) {
                rowServers.push_back(s.second);
            }

            std::sort(r.servers.begin(), r.servers.end(), [](std::pair<int, Server> a, std::pair<int, Server> b) {
                return a.second.power > b.second.power;
            });

            std::vector<std::pair<int, int>> unusedSlots;
            
            for (auto& s : r.servers) {
                if (m_dataCentre.rows[i].slotUsage + s.second.size > m_dataCentre.rowSize) break;
                
                int j = findValidFirstSlot(m_dataCentre.rows[i], s.second);
                if (j < 0) break;

                add(m_dataCentre.rows[i], s.second, j);
                auto& it = std::find_if(rowServers.begin(), rowServers.end(), [&s](Server server) {
                    return s.second.id == server.id;
                });
                if (it != rowServers.end()) {
                    rowServers.erase(it);
                }
            }
            availableServers.push_back(rowServers);
        }

        //for (auto& i : m_dataCentre.rows) {
        //    std::cout << (m_dataCentre.rowSize - i.unavailableSlots.size()) - i.slotUsage << std::endl;
        //}
        for (auto& i : m_dataCentre.rows) {
            std::cout << i.capacity << std::endl;
        }
        std::cout << std::endl << std::endl;

        for (int i = 0; i < m_dataCentre.rowCount; ++i) {
            Row r = m_dataCentre.rows[i];
            for (auto& availableSlot : r.availableSlots) {
                std::sort(availableServers[i].begin(), availableServers[i].end(), [](Server a, Server b) {
                    return a.capacity > b.capacity;
                });

                for (Server& server : availableServers[i]) {
                    if (server.size <= availableSlot.second) {
                        add(m_dataCentre.rows[i], server, availableSlot.first);
                        break;
                    }
                }
            }
        }

        //for (auto& i : m_dataCentre.rows) {
        //    std::cout << (m_dataCentre.rowSize - i.unavailableSlots.size()) - i.slotUsage << std::endl;
        //}
        for (auto& i : m_dataCentre.rows) {
            std::cout << i.capacity << std::endl;
        }
        std::cout << std::endl << std::endl;
    }

    DataCentre getResult() {
        return m_dataCentre;
    }
private:
    int findValidFirstSlot(Row r, Server s) {
        for (std::pair<int, int> availableSlot : r.availableSlots) {
            if (availableSlot.second > s.size) {
                return availableSlot.first;
            }
        }
        return -1;
    }

    DataCentre m_dataCentre;
    std::vector<Row> m_idealRows;
};
class PoolSolver {
public:
    void init(DataCentre dc) {
        m_dataCentre = dc;
    }

    void solve() {        
        for (Row r : m_dataCentre.rows) {
            for (std::pair<int, Server> s : r.servers) {
                int poolCapacity = INT_MAX;
                int poolID = -1;

                for (int i = 0; i < m_dataCentre.pools.size(); ++i) {
                    Pool p = m_dataCentre.pools[i];
                    if (p.capacity < poolCapacity) {
                        poolID = i;
                        poolCapacity = p.capacity;
                    }
                }

                add(m_dataCentre.pools[poolID], s.second);
            }
        }

        for (Pool p : m_dataCentre.pools) {
            std::cout << p.capacity << std::endl;
        }
    }

     DataCentre getResult() {
        return m_dataCentre;
    }
private:
    DataCentre m_dataCentre;
};

int main() {
    DataCentreLoader loader;
    loader.init("dc.in");
    DataCentre dataCentre = loader.getDataCentre();

    IdealSolver is;
    is.init(dataCentre);
    is.solve();
    std::vector<Row> rows = is.getResult();

    BinSolver bs;
    bs.init(dataCentre, rows);
    bs.solve();
    dataCentre = bs.getResult();

    PoolSolver ps;
    ps.init(dataCentre);
    ps.solve();
    dataCentre = ps.getResult();

    std::cout << "IAGYSD";

    return 0;
}