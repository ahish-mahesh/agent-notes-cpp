#include "TranscriptDeduplicator.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cmath>

TranscriptDeduplicator::TranscriptDeduplicator()
    : config_{}
{
}

TranscriptDeduplicator::TranscriptDeduplicator(const Config& config)
    : config_(config)
{
}

TranscriptDeduplicator::Segment TranscriptDeduplicator::processSegment(const Segment& newSegment)
{
    if (newSegment.text.empty())
    {
        return newSegment;
    }

    Segment processedSegment = newSegment;

    // Check for overlaps with recent segments
    for (auto it = recentSegments_.rbegin(); it != recentSegments_.rend(); ++it)
    {
        const Segment& previousSegment = *it;
        
        // Skip if segments don't overlap in time
        if (!hasTemporalOverlap(previousSegment, processedSegment))
        {
            continue;
        }

        // Detect content overlap
        OverlapInfo overlap = detectOverlap(previousSegment, processedSegment);
        
        if (overlap.hasOverlap)
        {
            // Resolve conflict using confidence and timestamp
            if (resolveConflict(previousSegment, processedSegment))
            {
                // Keep current segment, remove overlap
                processedSegment = removeOverlap(processedSegment, overlap);
            }
            else
            {
                // Previous segment wins, current becomes empty or partial
                processedSegment = removeOverlap(processedSegment, overlap);
            }
            
            // Only check against the most recent overlapping segment
            break;
        }
    }

    // Add to recent segments if not empty
    if (!processedSegment.text.empty())
    {
        recentSegments_.push_back(processedSegment);
        
        // Maintain maximum context size
        if (recentSegments_.size() > config_.maxContextSegments)
        {
            recentSegments_.pop_front();
        }
    }

    return processedSegment;
}

void TranscriptDeduplicator::clearContext()
{
    recentSegments_.clear();
}

TranscriptDeduplicator::OverlapInfo TranscriptDeduplicator::detectOverlap(
    const Segment& previous, const Segment& current) const
{
    OverlapInfo overlap;

    std::vector<std::string> prevWords = splitWords(previous.text);
    std::vector<std::string> currWords = splitWords(current.text);

    if (prevWords.empty() || currWords.empty())
    {
        return overlap;
    }

    // Use sliding window to find best overlap
    size_t windowSize = std::min(config_.slidingWindowSize, 
                                std::min(prevWords.size(), currWords.size()));

    double bestSimilarity = 0.0;
    size_t bestPrevStart = 0, bestCurrStart = 0, bestLength = 0;

    // Check overlaps at the end of previous segment with start of current
    for (size_t prevStart = prevWords.size() >= windowSize ? prevWords.size() - windowSize : 0; 
         prevStart <= prevWords.size() - 1; ++prevStart)
    {
        for (size_t currStart = 0; currStart <= currWords.size() - 1; ++currStart)
        {
            // Try different overlap lengths
            size_t maxLength = std::min(prevWords.size() - prevStart, currWords.size() - currStart);
            maxLength = std::min(maxLength, windowSize);

            for (size_t length = 1; length <= maxLength; ++length)
            {
                std::string prevText = joinWords(prevWords, prevStart, prevStart + length);
                std::string currText = joinWords(currWords, currStart, currStart + length);

                double similarity = calculateSimilarity(prevText, currText);

                if (similarity > bestSimilarity && similarity >= config_.overlapThreshold)
                {
                    bestSimilarity = similarity;
                    bestPrevStart = prevStart;
                    bestCurrStart = currStart;
                    bestLength = length;
                }
            }
        }
    }

    if (bestSimilarity >= config_.overlapThreshold)
    {
        overlap.hasOverlap = true;
        overlap.prevWordStart = bestPrevStart;
        overlap.prevWordEnd = bestPrevStart + bestLength;
        overlap.currWordStart = bestCurrStart;
        overlap.currWordEnd = bestCurrStart + bestLength;
        overlap.similarity = bestSimilarity;
    }

    return overlap;
}

double TranscriptDeduplicator::calculateSimilarity(const std::string& text1, const std::string& text2) const
{
    if (text1.empty() && text2.empty())
    {
        return 1.0;
    }

    if (text1.empty() || text2.empty())
    {
        return 0.0;
    }

    // Exact match
    if (text1 == text2)
    {
        return 1.0;
    }

    // Case-insensitive comparison
    std::string lower1 = text1, lower2 = text2;
    std::transform(lower1.begin(), lower1.end(), lower1.begin(), ::tolower);
    std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::tolower);

    if (lower1 == lower2)
    {
        return 0.95; // Slightly lower for case differences
    }

    if (!config_.enableFuzzyMatching)
    {
        return 0.0;
    }

    // Fuzzy matching using Levenshtein distance
    size_t distance = levenshteinDistance(lower1, lower2);
    size_t maxLength = std::max(lower1.length(), lower2.length());

    if (maxLength == 0)
    {
        return 1.0;
    }

    return 1.0 - (static_cast<double>(distance) / maxLength);
}

size_t TranscriptDeduplicator::levenshteinDistance(const std::string& s1, const std::string& s2) const
{
    size_t len1 = s1.length();
    size_t len2 = s2.length();

    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));

    // Initialize base cases
    for (size_t i = 0; i <= len1; ++i)
    {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j)
    {
        dp[0][j] = j;
    }

    // Fill the DP table
    for (size_t i = 1; i <= len1; ++i)
    {
        for (size_t j = 1; j <= len2; ++j)
        {
            if (s1[i - 1] == s2[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else
            {
                dp[i][j] = 1 + std::min({dp[i - 1][j],     // deletion
                                        dp[i][j - 1],     // insertion
                                        dp[i - 1][j - 1]}); // substitution
            }
        }
    }

    return dp[len1][len2];
}

std::vector<std::string> TranscriptDeduplicator::splitWords(const std::string& text) const
{
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;

    while (iss >> word)
    {
        // Remove punctuation from word for comparison, but keep original
        words.push_back(word);
    }

    return words;
}

std::string TranscriptDeduplicator::joinWords(const std::vector<std::string>& words, 
                                             size_t start, size_t end) const
{
    if (start >= words.size())
    {
        return "";
    }

    end = std::min(end, words.size());
    
    std::ostringstream oss;
    for (size_t i = start; i < end; ++i)
    {
        if (i > start)
        {
            oss << " ";
        }
        oss << words[i];
    }

    return oss.str();
}

TranscriptDeduplicator::Segment TranscriptDeduplicator::removeOverlap(
    const Segment& current, const OverlapInfo& overlap) const
{
    if (!overlap.hasOverlap)
    {
        return current;
    }

    std::vector<std::string> words = splitWords(current.text);
    
    // Remove overlapping words from the beginning of current segment
    if (overlap.currWordStart == 0)
    {
        // Remove from start
        std::string cleanedText = joinWords(words, overlap.currWordEnd);
        
        if (cleanedText.empty())
        {
            return Segment("", current.startTime, current.endTime, current.confidence, current.language);
        }

        // Adjust timing proportionally
        double totalDuration = current.endTime - current.startTime;
        double removedRatio = static_cast<double>(overlap.currWordEnd) / words.size();
        double newStartTime = current.startTime + (totalDuration * removedRatio);

        return Segment(cleanedText, newStartTime, current.endTime, current.confidence, current.language);
    }
    else
    {
        // Remove from middle or end (less common case)
        std::string beforeOverlap = joinWords(words, 0, overlap.currWordStart);
        std::string afterOverlap = joinWords(words, overlap.currWordEnd);
        
        std::string cleanedText = beforeOverlap;
        if (!beforeOverlap.empty() && !afterOverlap.empty())
        {
            cleanedText += " " + afterOverlap;
        }
        else if (!afterOverlap.empty())
        {
            cleanedText = afterOverlap;
        }

        return Segment(cleanedText, current.startTime, current.endTime, current.confidence, current.language);
    }
}

bool TranscriptDeduplicator::hasTemporalOverlap(const Segment& seg1, const Segment& seg2) const
{
    // Check if time ranges overlap
    return !(seg1.endTime <= seg2.startTime || seg2.endTime <= seg1.startTime);
}

bool TranscriptDeduplicator::resolveConflict(const Segment& previous, const Segment& current) const
{
    // Weighted decision based on confidence and timing
    double confidenceScore = current.confidence - previous.confidence;
    
    // Prefer more recent segments (slight bias toward current)
    double timeScore = 0.1; // Small bias for recency
    
    // Combined score
    double combinedScore = (config_.confidenceWeight * confidenceScore) + 
                          ((1.0 - config_.confidenceWeight) * timeScore);

    return combinedScore > 0.0; // Return true to keep current segment
}