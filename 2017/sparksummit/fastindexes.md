<!--open with Marp-->

<style>
.slide h2 {
color:#008dc8;
}
.slide   {
border-bottom-color:#008dc8;
border-bottom-style:solid;
border-bottom-width:10px;
}
.slide {
    background-repeat: no-repeat;
    background-position:  1% 99%;
background-image: url("sparksummit2017small.png");
}

</style>

<!-- *template: invert -->
<style>
 *[data-template~="invert"] {
color:white !important;
background-color:#008dc8 !important;
}
 *[data-template~="invert"] * {
color:white !important;
background-color:#008dc8 !important;
}
</style>


## ENGINEERING FAST INDEXES


<img src="sparksummit2017large.png" style="float:right; width:10%"/>

Daniel Lemire :maple_leaf:
https://lemire.me 

Joint work with lots of super smart people

<!--NSERC grant #26143-->


---

<!-- page_number: true -->

## Our recent work: Roaring Bitmaps

http://roaringbitmap.org/

Used by 
- Apache Spark,
- Netflix Atlas,
- LinkedIn Pinot,
- Apache Lucene, 
- Whoosh, 
- Metamarket's Druid
- eBay's Apache Kylin

Further reading:
<!--- <a href="https://techblog.king.com/player-segmentation-using-bitmap-data-structures/">Player segmentation using bitmap data structures</a> (at <a href="https://en.wikipedia.org/wiki/King_(company)">King Digital Entertainment</a>, the company behind <a href="https://en.wikipedia.org/wiki/Candy_Crush_Saga">Candy Crush</a>)-->
- <a href="https://www.elastic.co/blog/frame-of-reference-and-roaring-bitmaps">Frame of Reference and Roaring Bitmaps</a> (at Elastic, the company behind <a href="https://en.wikipedia.org/wiki/Elasticsearch">Elasticsearch</a>)


---

## Set data structures


We focus on sets of **integers**: $S= \{ 1,2,3, 1000 \}$. Ubiquitous in database or search engines.

- tests: $x \in S$?
- intersections: $S_2 \cap S_1$
- unions: $S_2 \cup S_1$
- differences: $S_2 \setminus S_1$
- Jaccard Index (Tanimoto similarity) $\vert S_1 \cap S_1 \vert  /\vert  S_1 \cup S_2\vert$

---

## "Ordered" Set 

- iterate 
  - in sorted order, 
  - in reverse order, 
  - skippable iterators (jump to first value $\geq x$)
- Rank: how many elements of the set are smaller than $k$?
- Select: find the k<sup>th</sup> smallest value
- Min/max: find the maximal and minimal value


---

## How do we implement integer sets?

- hash sets (``java.util.HashSet<Integer>``, ``std::unordered_set<uint32_t>``)
- sorted arrays (``std::vector<uint32_t>``)
- $\ldots$
- bitsets (``java.util.BitSet``)
- :heart: :heart: :heart: **compressed bitsets** :heart: :heart: :heart:


---

## What is a bitset???

Efficient way to represent a set of integers. 

E.g., 0, 1, 3, 4 becomes ``0b11011`` or "27".

Also called a "bitmap" or a "bit array".


```
add(x) {
  array[x / 64] |= (1 << (x % 64))
}

contains(x) {
  return array[x / 64] & (1 << (x % 64))
}
```

---

## How fast can you set bits in a bitset?

Very fast! Roughly three instructions (on x64)...

```
index = x / 64         -> a single shift
mask = 1 << ( x % 64)  -> a single shift
array[ index ] |- mask -> a logical OR to memory
```
(Or can use BMI's ``bts``.)

On recent x64 can set one bit every $\approx 1.65$ cycles (in cache)

---

## Bitsets are efficient

Intersection between {0, 1, 3} and {1, 3}
can be computed as AND operation between
``0b1011`` and ``0b1010``.

SHIT SHIT SHIT SHIT
Explain how fast this works, also include popcnt analysis.
SHIT SHIT SHIT SHIT


*Bit-level parallelism*.

Enables *Branchless* processing.

Economical: A **single byte** can represent *any* subset of {0, 1, 2, 3, 4, 5, 6, 7}.

---

## Bitsets are vectorizable

Logical ORs, ANDs, ANDNOTs, XORs can be computed *fast* with Single instruction, multiple data (SIMD) instructions.

- Intel Cannonlake (late 2017), AVX-512
  - Operate on 64 bytes with ONE instruction 
  - $\to$ **Several** 512-bit ops/cycle :cupid:
  - Java 9's Hotspot can use AVX 512
- ARM v8-A to get Scalable Vector Extension... 
  - up to 2048 bits!!!

Sadly... In Java's HotSpot mostly used copy arrays. :frowning:

<!--http://bugs.java.com/bugdatabase/view_bug.do?bug_id=8076276-->


---

## Bitsets are vectorizable... sadly...


Java's hotspot is limited in what it can autovectorize:

1. Copying arrays
2. String.indexOf
3. 
Logical ORs, ANDs, ANDNOTs, XORs can be computed *fast* with Single instruction, multiple data (SIMD) instructions.

- Intel Cannonlake (late 2017), AVX-512
  - Operate on 64 bytes with ONE instruction 
  - $\to$ **Several** 512-bit ops/cycle
  - Java 9's Hotspot can use AVX 512
- ARM v8-A to get Scalable Vector Extension... 
  - up to 2048 bits!!!

Sadly...

---

## Bitsets can be inefficient

Relatively wasteful to represent {1, 32000, 64000} with a bitset.

So we use compression...

---

## Memory usage (example 1)

dataset : weather_sept_85

| format                      | bits per value|
| ---------------------------- | -----:|
| hash sets (``std::unordered_set``) | 220 |
| arrays                       |   32 |
| bitsets                      |   15.26|
| compressed bitsets (Roaring) |   5.38 |


https://github.com/RoaringBitmap/CBitmapCompetition


---

## Performance: union + cardinality (example 1)

dataset : weather_sept_85


| format                       | CPU cycles per value|
| ---------------------------- | -----:|
| hash sets (``std::unordered_set``) | 300 |
| arrays                       |   8 |
| bitsets                      |   0.6|
| compressed bitsets (Roaring) |   0.6 |



https://github.com/RoaringBitmap/CBitmapCompetition


---

## Memory usage (example 2)

dataset : census1881_srt


| format                      | bits per value|
| ---------------------------- | -----:|
| hash sets (``std::unordered_set``) | 200 |
| arrays                       |   30 |
| bitsets                      |   900 |
| compressed bitsets (Roaring) |   2 |


https://github.com/RoaringBitmap/CBitmapCompetition


---

## Performance: union + cardinality (example 2)

dataset : census1881_srt



| format                       | CPU cycles per value|
| ---------------------------- | -----:|
| hash sets (``std::unordered_set``) | 200 |
| arrays                       |   6 |
| bitsets                      |   30|
| compressed bitsets (Roaring) |   1 |


https://github.com/RoaringBitmap/CBitmapCompetition


---

## What is happening? (Bitsets)


Bitsets are often best... except if data is 
very sparse (lots of 0s). Then you spend a
lot of time scanning zeros.

Threshold? ~1:100


---

## Hash sets do not scale!

Hash sets have great one-value look-up. But
if you have poor **data locality**...

```
  h1 <- some hash set
  h2 <- some hash set
  ...
  for(x in h1) {
     insert x in h2 // "sure" to hit a new cache line!!!!
```

Big hash sets mess with your cache! The bigger the hash sets, the worse things are!


---

## What is happening? (Arrays)

Arrays are your friends. Reliable. Simple. Economical.

But... **binary search** is *branchy* and has *bad locality*...

```
    while (low <= high) {
      int middleIndex = (low + high) >>> 1;
      int middleValue = array.get(middleIndex);

      if (middleValue < ikey) {
        low = middleIndex + 1;
      } else if (middleValue > ikey) {
        high = middleIndex - 1;
      } else {
        return middleIndex;
      }
    }
    return -(low + 1);
```


---

## Performance: value lookups ($x \in S$)

dataset : weather_sept_85


| format                       | CPU cycles per query|
| ---------------------------- | -----:|
| hash sets (``std::unordered_set``) | 50 |
| arrays                       |   900 |
| bitsets                      |    4|
| compressed bitsets (Roaring) |   80 |



---

## How do you compress bitsets?

- We have long runs of 0s or 1s.
- Use run-length encoding (RLE)

Example: $000000001111111100$ can be coded as 
<5><0> <5><1> <2><0>
using the format < number of repetitions >< value being repeated >

---

## RLE-compressed bitsets

- Oracle's BBC
- WAH (FastBit)
- EWAH (Git + Apache Hive)
- Concise (Druid)
- - $\ldots$

Further reading:
http://githubengineering.com/counting-objects/

---

## Downsides of RLE

"Complex" algorithms:
- data dependencies (hard to skip)
- lots of branches
- random access?


---

## Going back to performance: union + cardinality 

dataset : weather_sept_85



| format                       | CPU cycles per value|
| ---------------------------- | -----:|
| bitsets                      |   0.6|
| WAH                      |   4|
| EWAH                      |   2|
| Concise                      |   5|
| Roaring |   0.6 |

https://github.com/RoaringBitmap/CBitmapCompetition

---

## Better than RLE: Hybrid Model

Decompose 32-bit space into
16-bit spaces (chunk).

For each chunk, use best container:

Within each subspace use either...
- a sorted array ({1,20,144})
- a bitset (0b10000101011)
- a sequences of sorted runs ([0,10],[15,20])

That's Roaring!


---

## Bitset vs. Bitset...

Intersection: First compute the cardinality of the result. If low, use an array for the result (slow), otherwise generate a bitset (fast).

Union: Always generate a bitset (fast).


---

## Array vs. Array...

Intersection: Always an array. Use galloping if the sizes differs.

ADD CODE

Union: If sum of cardinalities is large, go for a bitset. Revert to an array if we got it wrong.

ADD CODE

---

## Array vs. Bitmap...

Intersection: Always an array. Very fast.

```
answer = new array
for value in array {
  if value in bitset {// branch but no data dependency
    append value to answer
  }
}
```

Union: Always a bitset. Very fast.


```
answer = clone the bitset
for value in array { // branchless
  set bit in answer at index value
}
```

---

<!-- *template: invert -->


## Go try it out!


- Java, Go, C, C++, C#, Rust, Python... (soon: Swift)
- http://roaringbitmap.org
- Documented interoperable serialized format.
- Free. Well-tested. 
- Wide community.