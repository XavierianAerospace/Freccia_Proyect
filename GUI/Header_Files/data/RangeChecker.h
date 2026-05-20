#ifndef RANGECHECKER_H
#define RANGECHECKER_H

#include <QString>
#include <map>

class RangeChecker {
public:
    enum AlertLevel {
        OK = 0,
        Warning = 1,
        Critical = 2
    };

    struct Range {
        double min_warn;
        double max_warn;
        double min_crit;
        double max_crit;
    };

    RangeChecker() {
        // Default ranges
        m_ranges["Roll"] = {-30.0, 30.0, -45.0, 45.0};
        m_ranges["Pitch"] = {-30.0, 30.0, -45.0, 45.0};
        m_ranges["Yaw"] = {-45.0, 45.0, -60.0, 60.0};
        m_ranges["Satélites"] = {5.5, 100.0, 3.5, 100.0};
        m_ranges["HDOP"] = {0.0, 2.0, 0.0, 5.0};
        m_ranges["Temperatura"] = {0.0, 45.0, -10.0, 60.0};
        m_ranges["Presión"] = {800.0, 1200.0, 700.0, 1300.0};
        m_ranges["AltDiff"] = {-10.0, 500.0, -20.0, 1000.0};
    }

    void setRange(const QString& name, const Range& r) {
        m_ranges[name] = r;
    }

    AlertLevel check(const QString& name, double value) {
        auto it = m_ranges.find(name);
        if (it == m_ranges.end()) return OK;

        const Range& r = it->second;
        if (name == "Satélites") {
            if (value < r.min_crit) return Critical;
            if (value < r.min_warn) return Warning;
            return OK;
        }
        if (name == "HDOP") {
            if (value > r.max_crit) return Critical;
            if (value > r.max_warn) return Warning;
            return OK;
        }

        if (value < r.min_crit || value > r.max_crit) return Critical;
        if (value < r.min_warn || value > r.max_warn) return Warning;

        return OK;
    }

private:
    std::map<QString, Range> m_ranges;
};

#endif // RANGECHECKER_H
