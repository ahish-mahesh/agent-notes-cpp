#pragma once

#include <vector>
#include <string>
#include <deque>

/**
 * @brief Transcript deduplication and cleanup utility
 * 
 * Handles removal of duplicate text from overlapping audio segments using:
 * - Timestamp alignment
 * - Sliding window overlap detection
 * - Confidence scoring for conflict resolution
 */
class TranscriptDeduplicator
{
public:
    /**
     * @brief Configuration for deduplication behavior
     */
    struct Config
    {
        size_t slidingWindowSize = 10;    ///< Number of words to compare for overlap detection
        double overlapThreshold = 0.7;   ///< Similarity threshold for considering segments overlapping (0.0-1.0)
        double confidenceWeight = 0.3;   ///< Weight given to confidence vs timestamp in conflict resolution
        size_t maxContextSegments = 5;   ///< Maximum number of previous segments to keep for context
        bool enableFuzzyMatching = true; ///< Enable fuzzy string matching for near-duplicates
    };

    /**
     * @brief Transcript segment with metadata
     */
    struct Segment
    {
        std::string text;
        double startTime;
        double endTime;
        float confidence;
        std::string language;

        // Constructor for easy creation
        Segment(const std::string& t, double start, double end, float conf, const std::string& lang = "")
            : text(t), startTime(start), endTime(end), confidence(conf), language(lang) {}
    };

    /**
     * @brief Constructor with default config
     */
    TranscriptDeduplicator();

    /**
     * @brief Constructor
     * @param config Deduplication configuration
     */
    explicit TranscriptDeduplicator(const Config& config);

    /**
     * @brief Process new segment and remove duplicates
     * @param newSegment New transcript segment to process
     * @return Deduplicated segment (may be empty if completely duplicate)
     */
    Segment processSegment(const Segment& newSegment);

    /**
     * @brief Clear all stored context
     */
    void clearContext();

    /**
     * @brief Get current configuration
     * @return Current deduplication configuration
     */
    const Config& getConfig() const { return config_; }

    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const Config& config) { config_ = config; }

private:
    Config config_;
    std::deque<Segment> recentSegments_;

    /**
     * @brief Detect overlapping content between segments
     * @param previous Previous segment
     * @param current Current segment
     * @return Overlap information (start index, end index, similarity score)
     */
    struct OverlapInfo
    {
        size_t prevWordStart = 0;
        size_t prevWordEnd = 0;
        size_t currWordStart = 0;
        size_t currWordEnd = 0;
        double similarity = 0.0;
        bool hasOverlap = false;
    };

    OverlapInfo detectOverlap(const Segment& previous, const Segment& current) const;

    /**
     * @brief Calculate similarity between two text segments
     * @param text1 First text segment
     * @param text2 Second text segment
     * @return Similarity score (0.0-1.0)
     */
    double calculateSimilarity(const std::string& text1, const std::string& text2) const;

    /**
     * @brief Calculate Levenshtein distance between two strings
     * @param s1 First string
     * @param s2 Second string
     * @return Edit distance
     */
    size_t levenshteinDistance(const std::string& s1, const std::string& s2) const;

    /**
     * @brief Split text into words
     * @param text Input text
     * @return Vector of words
     */
    std::vector<std::string> splitWords(const std::string& text) const;

    /**
     * @brief Join words with spaces
     * @param words Vector of words
     * @param start Start index (inclusive)
     * @param end End index (exclusive)
     * @return Joined text
     */
    std::string joinWords(const std::vector<std::string>& words, size_t start = 0, size_t end = SIZE_MAX) const;

    /**
     * @brief Remove overlapping content from current segment
     * @param current Current segment
     * @param overlap Overlap information
     * @return Cleaned segment
     */
    Segment removeOverlap(const Segment& current, const OverlapInfo& overlap) const;

    /**
     * @brief Check if segments overlap in time
     * @param seg1 First segment
     * @param seg2 Second segment
     * @return True if segments overlap in time
     */
    bool hasTemporalOverlap(const Segment& seg1, const Segment& seg2) const;

    /**
     * @brief Resolve conflict between overlapping segments
     * @param previous Previous segment
     * @param current Current segment
     * @return Which segment to prefer (true = keep current, false = keep previous)
     */
    bool resolveConflict(const Segment& previous, const Segment& current) const;
};