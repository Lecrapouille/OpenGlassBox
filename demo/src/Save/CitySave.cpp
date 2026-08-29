//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Save/CitySave.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "OpenGlassBox/Script/Lexer.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb
{
namespace
{

uint32_t rotr(uint32_t value, uint32_t bits)
{
    return (value >> bits) | (value << (32u - bits));
}

std::string sha256(std::string const& input)
{
    static uint32_t const K[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
        0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
        0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
        0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
        0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
        0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
        0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
        0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
        0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
        0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
        0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
    };

    std::vector<uint8_t> data(input.begin(), input.end());
    uint64_t const bitLen = data.size() * 8u;
    data.push_back(0x80u);
    while ((data.size() % 64u) != 56u)
        data.push_back(0u);
    for (int i = 7; i >= 0; --i)
        data.push_back(uint8_t((bitLen >> (i * 8)) & 0xffu));

    uint32_t h[8] = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                      0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };

    for (size_t chunk = 0u; chunk < data.size(); chunk += 64u)
    {
        uint32_t w[64];
        for (size_t i = 0u; i < 16u; ++i)
        {
            size_t const o = chunk + i * 4u;
            w[i] = (uint32_t(data[o]) << 24u) |
                   (uint32_t(data[o + 1u]) << 16u) |
                   (uint32_t(data[o + 2u]) << 8u) | uint32_t(data[o + 3u]);
        }
        for (size_t i = 16u; i < 64u; ++i)
        {
            uint32_t const s0 = rotr(w[i - 15u], 7u) ^ rotr(w[i - 15u], 18u) ^
                                (w[i - 15u] >> 3u);
            uint32_t const s1 = rotr(w[i - 2u], 17u) ^ rotr(w[i - 2u], 19u) ^
                                (w[i - 2u] >> 10u);
            w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (size_t i = 0u; i < 64u; ++i)
        {
            uint32_t const S1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
            uint32_t const ch = (e & f) ^ ((~e) & g);
            uint32_t const temp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t const S0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
            uint32_t const maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t const temp2 = S0 + maj;
            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    char hex[65];
    for (int i = 0; i < 8; ++i)
        std::snprintf(hex + i * 8, 9, "%08x", h[i]);
    return std::string(hex, 64u);
}

std::string fileBasename(std::string const& path)
{
    size_t const slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1u);
}

void writeResources(std::ostream& out, Resources const& resources)
{
    out << "[";
    for (Resource const& resource : resources.getAll())
    {
        if (resource.getAmount() == 0u)
            continue;
        out << " " << resource.getTypeName() << " " << resource.getAmount();
    }
    out << " ]";
}

bool expect(Lexer& lexer, char const* word, std::string& error)
{
    Token const token = lexer.next();
    if (token.text != word)
    {
        error = "Expected '" + std::string(word) + "' but read '" + token.text +
                "'";
        return false;
    }
    return true;
}

bool readUint(Lexer& lexer, uint32_t& value, std::string& error)
{
    Token const token = lexer.next();
    if (!token.valid())
    {
        error = "Expected an integer";
        return false;
    }
    try
    {
        value = uint32_t(std::stoul(token.text));
    }
    catch (...)
    {
        error = "Not an integer: " + token.text;
        return false;
    }
    return true;
}

bool readInt(Lexer& lexer, int32_t& value, std::string& error)
{
    Token const token = lexer.next();
    if (!token.valid())
    {
        error = "Expected an integer";
        return false;
    }
    try
    {
        value = int32_t(std::stol(token.text));
    }
    catch (...)
    {
        error = "Not an integer: " + token.text;
        return false;
    }
    return true;
}

bool readFloat(Lexer& lexer, float& value, std::string& error)
{
    Token const token = lexer.next();
    if (!token.valid())
    {
        error = "Expected a number";
        return false;
    }
    try
    {
        value = std::stof(token.text);
    }
    catch (...)
    {
        error = "Not a number: " + token.text;
        return false;
    }
    return true;
}

bool readResources(Lexer& lexer, Resources& resources, std::string& error)
{
    if (!expect(lexer, "[", error))
        return false;
    while (lexer.peek().valid() && (lexer.peek().text != "]"))
    {
        Token const type = lexer.next();
        uint32_t amount = 0u;
        if (!readUint(lexer, amount, error))
            return false;
        resources.addResource(type.text, amount);
    }
    return expect(lexer, "]", error);
}

bool parseHeader(Lexer& lexer, CitySaveHeader& header, std::string& error)
{
    if (!expect(lexer, "save", error))
        return false;
    while (lexer.peek().valid() && (lexer.peek().text != "end"))
    {
        Token const key = lexer.next();
        if (key.text == "ruleset")
        {
            header.ruleset = lexer.next().text;
        }
        else if (key.text == "hash")
        {
            header.hash = lexer.next().text;
        }
        else if (key.text == "types")
        {
            if (!expect(lexer, "[", error))
                return false;
            while (lexer.peek().valid() && (lexer.peek().text != "]"))
                header.types.push_back(lexer.next().text);
            if (!expect(lexer, "]", error))
                return false;
        }
        else
        {
            error = "Unknown save field '" + key.text + "'";
            return false;
        }
    }
    return expect(lexer, "end", error);
}

std::vector<std::string> usedTypes(Simulation const& simulation)
{
    std::vector<std::string> names;
    auto const add = [&](Name const& name)
    {
        if (name.empty())
            return;
        for (std::string const& existing : names)
        {
            if (existing == name.str())
                return;
        }
        names.push_back(name.str());
    };

    for (auto const& cityIt : simulation.getCities())
    {
        City const& city = *cityIt.second;
        for (auto const& pathIt : city.getPaths())
        {
            add(pathIt.second->getTypeName());
            for (auto const& segment : pathIt.second->getSegments())
                add(segment->getTypeName());
        }
        for (auto const& building: city.getBuildings())
            add(building->getTypeName());
        for (auto const& zone : city.getZones())
            add(zone->getTypeName());
        for (auto const& agent : city.getAgents())
            add(agent->getTypeName());
    }
    return names;
}

bool typeExists(Ruleset const& script, std::string const& name)
{
    return (script.findPathType(name) != nullptr) ||
           (script.findSegmentType(name) != nullptr) ||
           (script.findBuildingType(name) != nullptr) ||
           (script.findZoneType(name) != nullptr) ||
           (script.findAgentType(name) != nullptr) ||
           (script.findLayerType(name) != nullptr);
}

} // namespace

// -----------------------------------------------------------------------------
std::string CitySave::hashString(std::string const& text)
{
    return sha256(text);
}

// -----------------------------------------------------------------------------
std::string CitySave::hashFile(std::string const& path)
{
    std::ifstream file(path);
    if (!file)
        return {};
    std::stringstream buffer;
    buffer << file.rdbuf();
    return sha256(buffer.str());
}

// -----------------------------------------------------------------------------
bool CitySave::peekHeader(std::string const& path,
                          CitySaveHeader& header,
                          std::string& error)
{
    Lexer lexer;
    if (!lexer.openFile(path))
    {
        error = "Cannot open '" + path + "'";
        return false;
    }
    return parseHeader(lexer, header, error);
}

// -----------------------------------------------------------------------------
bool CitySave::matchesRuleset(CitySaveHeader const& header,
                              std::string const& rulesetPath,
                              std::string& error)
{
    std::string const actual = hashFile(rulesetPath);
    if (actual.empty())
    {
        error = "Cannot read ruleset '" + rulesetPath + "'";
        return false;
    }
    if (actual != header.hash)
    {
        error = "This save was written against a different version of '" +
                header.ruleset + "' (hash mismatch)";
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
bool CitySave::write(std::string const& path,
                     Simulation const& simulation,
                     std::string const& rulesetPath,
                     std::string& error)
{
    std::ofstream out(path);
    if (!out)
    {
        error = "Cannot write '" + path + "'";
        return false;
    }

    std::string const hash = hashFile(rulesetPath);
    out << "save\n";
    out << "\truleset " << fileBasename(rulesetPath) << "\n";
    out << "\thash " << hash << "\n";
    out << "\ttypes [";
    for (std::string const& type : usedTypes(simulation))
        out << " " << type;
    out << " ]\nend\n\n";

    out << "clock " << simulation.getClock().getTicks() << "\n\n";

    if (simulation.getCities().empty())
        return true;

    City const& city = *simulation.getCities().begin()->second;
    out << "city " << city.getName() << " size " << city.getRegion().sizeU
        << " " << city.getRegion().sizeV << "\n";
    out << "globals ";
    writeResources(out, city.getGlobals());
    out << "\n";

    for (auto const& pathIt : city.getPaths())
    {
        Path const& road = *pathIt.second;
        out << "path " << road.getTypeName() << "\n";
        for (auto const& node : road.getNodes())
        {
            Cell const cell = city.worldToCell(node->getPosition());
            out << "\tnode " << node->getId() << " " << cell.u << " "
                << cell.v << "\n";
        }
        for (auto const& segment : road.getSegments())
        {
            out << "\tsegment " << segment->getId() << " " << segment->getTypeName() << " "
                << segment->getFrom().getId() << " " << segment->getTo().getId() << " flow "
                << segment->getFlow() << "\n";
        }
        out << "end\n";
    }

    for (auto const& building : city.getBuildings())
    {
        out << "building " << building->getTypeName();
        if (building->getSegment() != nullptr)
        {
            out << " on segment " << building->getSegment()->getId() << " " << building->getSegmentOffset();
        }
        else if (building->getNode() != nullptr)
        {
            out << " on node " << building->getNode()->getId();
        }
        else
        {
            Cell const cell = city.worldToCell(building->getPosition());
            out << " at " << cell.u << " " << cell.v;
        }
        out << " resources ";
        writeResources(out, building->getResources());
        out << "\n";
    }

    for (auto const& zone : city.getZones())
    {
        CellRegion const& region = zone->getRegion();
        out << "zone " << zone->getTypeName() << " " << region.u0 << " " << region.v0
            << " " << region.sizeU << " " << region.sizeV << "\n";
    }

    for (auto const& layerIt : city.getLayers())
    {
        Layer const& layer = *layerIt.second;
        out << "layer " << layer.getTypeName() << "\n";
        layer.forEachCell(
            [&](int32_t u, int32_t v, uint32_t amount)
            { out << "\tcell " << u << " " << v << " " << amount << "\n"; });
        out << "end\n";
    }

    for (auto const& agent : city.getAgents())
    {
        out << "agent " << agent->getTypeName() << " to " << agent->getTarget()
            << " pos " << agent->getPosition().x << " " << agent->getPosition().y
            << " offset " << agent->getOffset();
        if (agent->getSegment() != nullptr)
            out << " segment " << agent->getSegment()->getId();
        if (agent->getPreviousNode() != nullptr)
            out << " last " << agent->getPreviousNode()->getId();
        out << " resources ";
        writeResources(out, agent->getResources());
        out << "\n";
    }

    return true;
}

// -----------------------------------------------------------------------------
bool CitySave::read(std::string const& filePath,
                    Simulation& simulation,
                    std::string& error)
{
    Lexer lexer;
    if (!lexer.openFile(filePath))
    {
        error = "Cannot open '" + filePath + "'";
        return false;
    }

    CitySaveHeader header;
    if (!parseHeader(lexer, header, error))
        return false;

    for (std::string const& type : header.types)
    {
        if (!typeExists(simulation.getRuleset(), type))
        {
            error = "This save expects type '" + type +
                    "'; the open ruleset does not define it";
            return false;
        }
    }

    Ruleset const& script = simulation.getRuleset();
    City* city = nullptr;
    Path* currentPath = nullptr;

    while (lexer.peek().valid())
    {
        Token const token = lexer.next();
        if (token.text == "clock")
        {
            uint32_t ticks = 0u;
            if (!readUint(lexer, ticks, error))
                return false;
            simulation.setTicks(ticks);
        }
        else if (token.text == "city")
        {
            Token const name = lexer.next();
            if (!expect(lexer, "size", error))
                return false;
            uint32_t sizeU = 0u;
            uint32_t sizeV = 0u;
            if (!readUint(lexer, sizeU, error) ||
                !readUint(lexer, sizeV, error))
                return false;
            city = &simulation.addCity(
                name.text, Vector3f(0.0f, 0.0f, 0.0f), sizeU, sizeV);
            for (auto const& it : script.getLayerTypes())
                city->addLayer(*it.second);
        }
        else if (token.text == "globals")
        {
            if (city == nullptr)
            {
                error = "globals before city";
                return false;
            }
            if (!readResources(lexer, city->getGlobals(), error))
                return false;
        }
        else if (token.text == "path")
        {
            if (city == nullptr)
            {
                error = "path before city";
                return false;
            }
            Token const name = lexer.next();
            currentPath = &city->addPath(script.getPathType(name.text));
            while (lexer.peek().valid() && (lexer.peek().text != "end"))
            {
                Token const kind = lexer.next();
                if (kind.text == "node")
                {
                    uint32_t id = 0u;
                    int32_t u = 0;
                    int32_t v = 0;
                    if (!readUint(lexer, id, error) ||
                        !readInt(lexer, u, error) || !readInt(lexer, v, error))
                    {
                        return false;
                    }
                    currentPath->addNode(id, city->cellToWorld({ u, v }));
                }
                else if (kind.text == "segment")
                {
                    uint32_t id = 0u;
                    uint32_t from = 0u;
                    uint32_t to = 0u;
                    if (!readUint(lexer, id, error))
                        return false;
                    Token const type = lexer.next();
                    if (!readUint(lexer, from, error) ||
                        !readUint(lexer, to, error))
                    {
                        return false;
                    }
                    Node* n1 = currentPath->findNode(from);
                    Node* n2 = currentPath->findNode(to);
                    if ((n1 == nullptr) || (n2 == nullptr))
                    {
                        error = "Unknown node in segment";
                        return false;
                    }
                    Segment& segment = currentPath->addSegment(
                        id, script.getSegmentType(type.text), *n1, *n2);
                    if (lexer.peek().text == "flow")
                    {
                        lexer.next();
                        float flow = 0.0f;
                        if (!readFloat(lexer, flow, error))
                            return false;
                        segment.setFlow(flow);
                    }
                }
                else
                {
                    error = "Unknown path item '" + kind.text + "'";
                    return false;
                }
            }
            if (!expect(lexer, "end", error))
                return false;
        }
        else if (token.text == "building")
        {
            if ((city == nullptr) || (city->getPaths().empty()))
            {
                error = "building before path";
                return false;
            }
            Token const type = lexer.next();
            Path* path = city->getPaths().begin()->second.get();
            Token const where = lexer.next();
            Building* building = nullptr;
            if ((where.text == "on") && (lexer.peek().text == "segment"))
            {
                lexer.next();
                uint32_t segmentId = 0u;
                float offset = 0.5f;
                if (!readUint(lexer, segmentId, error) ||
                    !readFloat(lexer, offset, error))
                    return false;
                Segment* segment = path->findSegment(segmentId);
                if (segment == nullptr)
                {
                    error = "Unknown segment for building";
                    return false;
                }
                building = &city->addBuilding(
                    script.getBuildingType(type.text), *path, *segment, offset);
            }
            else if ((where.text == "on") && (lexer.peek().text == "node"))
            {
                lexer.next();
                uint32_t nodeId = 0u;
                if (!readUint(lexer, nodeId, error))
                    return false;
                Node* node = path->findNode(nodeId);
                if (node == nullptr)
                {
                    error = "Unknown node for building";
                    return false;
                }
                building = &city->addBuilding(script.getBuildingType(type.text), *node);
            }
            else if (where.text == "at")
            {
                int32_t u = 0;
                int32_t v = 0;
                if (!readInt(lexer, u, error) || !readInt(lexer, v, error))
                    return false;
                building = &city->addBuilding(script.getBuildingType(type.text),
                                      city->cellToWorld({ u, v }));
            }
            else
            {
                error = "Expected on/at after building";
                return false;
            }
            if (lexer.peek().text == "resources")
            {
                lexer.next();
                Resources loaded;
                if (!readResources(lexer, loaded, error))
                    return false;
                if (building != nullptr)
                {
                    // Only the amounts come from the save. The capacities come
                    // from the ruleset, and overwriting them left every loaded
                    // building unable to hold anything.
                    building->getResources().setAmounts(loaded);
                }
            }
        }
        else if (token.text == "zone")
        {
            if (city == nullptr)
            {
                error = "zone before city";
                return false;
            }
            Token const type = lexer.next();
            int32_t u0 = 0;
            int32_t v0 = 0;
            uint32_t sizeU = 0u;
            uint32_t sizeV = 0u;
            if (!readInt(lexer, u0, error) || !readInt(lexer, v0, error) ||
                !readUint(lexer, sizeU, error) ||
                !readUint(lexer, sizeV, error))
            {
                return false;
            }
            city->addZone(script.getZoneType(type.text),
                          CellRegion{ u0, v0, sizeU, sizeV });
        }
        else if (token.text == "layer")
        {
            Token const name = lexer.next();
            Layer* layer = simulation.findLayer(name.text);
            while (lexer.peek().valid() && (lexer.peek().text != "end"))
            {
                if (!expect(lexer, "cell", error))
                    return false;
                int32_t u = 0;
                int32_t v = 0;
                uint32_t amount = 0u;
                if (!readInt(lexer, u, error) || !readInt(lexer, v, error) ||
                    !readUint(lexer, amount, error))
                {
                    return false;
                }
                if (layer != nullptr)
                    layer->setResource({ u, v }, amount);
            }
            if (!expect(lexer, "end", error))
                return false;
        }
        else if (token.text == "agent")
        {
            if ((city == nullptr) || city->getBuildings().empty())
            {
                error = "agent before building";
                return false;
            }
            Token const type = lexer.next();
            if (!expect(lexer, "to", error))
                return false;
            Token const target = lexer.next();
            Vector3f position;
            float offset = 0.0f;
            uint32_t segmentId = 0u;
            uint32_t lastId = 0u;
            bool hasSegment = false;
            bool hasLast = false;
            Resources cargo;
            while (lexer.peek().valid())
            {
                std::string const& look = lexer.peek().text;
                if ((look == "city") || (look == "path") || (look == "building") ||
                    (look == "zone") || (look == "layer") || (look == "agent") ||
                    (look == "clock"))
                {
                    break;
                }
                Token const key = lexer.next();
                if (key.text == "pos")
                {
                    if (!readFloat(lexer, position.x, error) ||
                        !readFloat(lexer, position.y, error))
                    {
                        return false;
                    }
                }
                else if (key.text == "offset")
                {
                    if (!readFloat(lexer, offset, error))
                        return false;
                }
                else if (key.text == "segment")
                {
                    hasSegment = readUint(lexer, segmentId, error);
                    if (!hasSegment)
                        return false;
                }
                else if (key.text == "last")
                {
                    hasLast = readUint(lexer, lastId, error);
                    if (!hasLast)
                        return false;
                }
                else if (key.text == "resources")
                {
                    if (!readResources(lexer, cargo, error))
                        return false;
                }
                else
                {
                    error = "Unknown agent field '" + key.text + "'";
                    return false;
                }
            }
            Agent& agent = city->addAgent(script.getAgentType(type.text),
                                          *city->getBuildings().front(),
                                          cargo,
                                          target.text);
            Segment* segment = nullptr;
            Node* last = nullptr;
            if (!city->getPaths().empty())
            {
                Path& path = *city->getPaths().begin()->second;
                if (hasSegment)
                    segment = path.findSegment(segmentId);
                if (hasLast)
                    last = path.findNode(lastId);
            }
            agent.relocate(position, segment, offset, last);
        }
        else
        {
            error = "Unknown save section '" + token.text + "'";
            return false;
        }
    }

    return city != nullptr;
}

} // namespace ogb
