Finds all hands in the list that are detected with at least a certain level of confidence.

For each hand, the Leap Motion device provides a level of confidence that the hand and fingers have been correctly interpreted, ranging from 1 (most confident) to 0 (not confident at all).

   - `Hands` — The list of hands to search in.
   - `Confidence` — The minimum level of confidence to search for.
   - `Found Hands` — All items from `Hands` that have at least the minimum level of confidence.
