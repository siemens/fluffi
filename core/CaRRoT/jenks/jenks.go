/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier
*/

§§package jenks
§§
§§// this code is a demo implementation of https://en.wikipedia.org/wiki/Jenks_natural_breaks_optimization
§§
§§import (
§§	"fmt"
§§	"math"
§§	"os"
§§	"runtime"
§§)
§§
§§type Groupable interface {
§§	Order() int
§§}
§§
§§func JenksGroup(data []Groupable, maxgroups int) [][2]int {
§§	// if !sort.IntsAreSorted(data) { // O(n)
§§	// 	sort.Ints(data) // O(n*log(n))
§§	// }
§§	// if len(data)%maxgroups != 0 {
§§	// 	fmt.Println("not cleanly divisible")
§§	// 	return [][2]int{}
§§	// }
§§
§§	var splits [][2]int = make([][2]int, maxgroups)
§§	increment := len(data) / maxgroups
§§	for i := 0; i < maxgroups; i++ { // O(k)
§§		splits[i] = [2]int{i * increment, (i + 1) * increment}
§§	}
§§
§§	//fmt.Println("PRE SPLITS:", splits)
§§	// for _, i := range splits {
§§	// 	fmt.Println(data[i[0]:i[1]])
§§	// }
§§
§§	// calculate class medians
§§	var medians []float64 = make([]float64, maxgroups)
§§	for i := 0; i < maxgroups; i++ { // O(n)
§§		for _, elm := range data[splits[i][0]:splits[i][1]] {
§§			medians[i] += float64(elm.Order())
§§		}
§§		medians[i] = medians[i] / float64(splits[i][1]-splits[i][0])
§§	}
§§	//fmt.Println("PRE MEDIANS:", medians)
§§
§§	for cnt := 0; cnt < len(data)*len(data); cnt++ { // O(n(k+abislwas))
§§		wastouched := 0
§§		touched := make(map[int]int, maxgroups)
§§		set := make(map[int]struct{}, maxgroups*2)
§§
§§		//fmt.Println("outer iteration")
§§		// calculate difference of first and last element to own and adjacent classes, maybe move.
§§		for i := 0; i < maxgroups; i++ {
§§			//fmt.Println("inner iteration")
§§			if splits[i][1]-splits[i][0] == 0 {
§§				// ignore this split, it will stay empty
§§				continue
§§			} else if splits[i][1]-splits[i][0] == 1 {
§§				// we should really not have classes with only one element.
§§				// fixes: we might move this to an adjacent class
§§				lo := getNextLowerNonEmpty(splits, i)
§§				hi := getNextHigherNonEmpty(splits, i)
§§				if i > 0 && i < maxgroups-1 {
§§					if lo != -1 && hi != -1 {
§§						higherdeviation := math.Abs(medians[hi] - float64(data[splits[i][0]].Order()))
§§						lowerdeviation := math.Abs(medians[lo] - float64(data[splits[i][0]].Order()))
§§						if higherdeviation < lowerdeviation {
§§							// move up
§§							// move the element up
§§							touched[hi] = i
§§							set[hi] = struct{}{}
§§							set[i] = struct{}{}
§§							if val, cont := touched[i]; cont && val == hi {
§§								//fmt.Println("moved that one before")
§§								wastouched--
§§							} else {
§§								wastouched++
§§							}
§§							splits[i][1]--
§§							splits[hi][0]--
§§							// to be acurate, we should recalculate the median after a move
§§							//redoMedians([]int{i, hi}, medians, splits, data)
§§						} else {
§§							// move down
§§							// move the element down
§§							touched[lo] = i
§§							set[lo] = struct{}{}
§§							set[i] = struct{}{}
§§							if val, cont := touched[i]; cont && val == lo {
§§								//fmt.Println("moved that one before")
§§								wastouched--
§§							} else {
§§								wastouched++
§§							}
§§							splits[i][0]++
§§							splits[lo][1]++
§§							// to be acurate, we should recalculate the median after a move
§§							//redoMedians([]int{i, lo}, medians, splits, data)
§§						}
§§					} else if lo != -1 {
§§						fmt.Println("BAD HIT that is not as bad as it used to be")
§§						// move down
§§						// move the element down
§§						touched[lo] = i
§§						set[lo] = struct{}{}
§§						set[i] = struct{}{}
§§						if val, cont := touched[i]; cont && val == lo {
§§							//fmt.Println("moved that one before")
§§							wastouched--
§§						} else {
§§							wastouched++
§§						}
§§						splits[i][0]++
§§						splits[lo][1]++
§§						// to be acurate, we should recalculate the median after a move
§§						//redoMedians([]int{i, lo}, medians, splits, data)
§§					} else if hi != -1 {
§§						fmt.Println("BAD HIT that is not as bad as it used to be")
§§						// move up
§§						// move the element up
§§						touched[hi] = i
§§						set[hi] = struct{}{}
§§						set[i] = struct{}{}
§§						if val, cont := touched[i]; cont && val == hi {
§§							//fmt.Println("moved that one before")
§§							wastouched--
§§						} else {
§§							wastouched++
§§						}
§§						splits[i][1]--
§§						splits[hi][0]--
§§						// to be acurate, we should recalculate the median after a move
§§						//redoMedians([]int{i, hi}, medians, splits, data)
§§					} else {
§§						fmt.Println("THIS SHOULD NOT HAPPEN. THIS IS BAD.")
§§					}
§§
§§				} else {
§§					// first or last grouping only has one element. we ignore this.
§§					//fmt.Println("BAD HIT")
§§				}
§§			} else if splits[i][1]-splits[i][0] < 0 {
§§				fmt.Println("smaller than zero split, this is an error")
§§				os.Exit(2)
§§			} else if splits[i][1]-splits[i][0] > 1 {
§§				// low element
§§				if i > 0 {
§§					n := getNextLowerNonEmpty(splits, i)
§§					currentdeviation := math.Abs(medians[i] - float64(data[splits[i][0]].Order()))
§§					adjacentdeviation := math.Abs(medians[n] - float64(data[splits[i][0]].Order()))
§§					if adjacentdeviation < currentdeviation {
§§						// move the element down
§§						touched[n] = i
§§						set[n] = struct{}{}
§§						set[i] = struct{}{}
§§						if val, cont := touched[i]; cont && val == n {
§§							//fmt.Println("moved that one before")
§§							wastouched--
§§						} else {
§§							wastouched++
§§						}
§§						//fmt.Println("moving", i, n, "down")
§§						splits[i][0]++
§§						splits[n][1]++
§§						// to be acurate, we should recalculate the median after a move
§§						//redoMedians([]int{i, n}, medians, splits, data)
§§					}
§§
§§				}
§§				// high element
§§				if i < maxgroups-1 {
§§					n := getNextHigherNonEmpty(splits, i)
§§					currentdeviation := math.Abs(medians[i] - float64(data[splits[i][1]].Order()))
§§					adjacentdeviation := math.Abs(medians[n] - float64(data[splits[i][1]].Order()))
§§					if adjacentdeviation < currentdeviation {
§§						// move the element up
§§						touched[n] = i
§§						set[n] = struct{}{}
§§						set[i] = struct{}{}
§§						if val, cont := touched[i]; cont && val == n {
§§							//fmt.Println("moved that one before")
§§							wastouched--
§§						} else {
§§							wastouched++
§§						}
§§						//fmt.Println("moving", i, n, "up")
§§						splits[i][1]--
§§						splits[n][0]--
§§						// to be acurate, we should recalculate the median after a move
§§						//redoMedians([]int{i, n}, medians, splits, data)
§§					}
§§
§§				}
§§			}
§§		}
§§		// test redoing medians here
§§		redoMedians(set, medians, splits, data)
§§
§§		//fmt.Println("INTER SPLITS:", splits)
§§		//fmt.Println("INTER MEDIANS:", medians)
§§		if wastouched == 0 {
§§			//fmt.Println("breaking @", cnt)
§§			break
§§		}
§§	}
§§
§§	// fmt.Println("POST SPLITS:", splits)
§§	// fmt.Println("POST MEDIANS:", medians)
§§
§§	var ret [][2]int
§§	for i, spl := range splits {
§§		if spl[0] != spl[1] {
§§			ret = append(ret, splits[i])
§§		}
§§	}
§§
§§	return ret
§§}
§§
§§func getNextLowerNonEmpty(splits [][2]int, i int) int {
§§	for {
§§		i--
§§		if i < 0 {
§§			return -1
§§		}
§§		if splits[i][1]-splits[i][0] > 0 {
§§			return i
§§		}
§§	}
§§}
§§func getNextHigherNonEmpty(splits [][2]int, i int) int {
§§	for {
§§		i++
§§		if i >= len(splits) {
§§			return -1
§§		}
§§		if splits[i][1]-splits[i][0] > 0 {
§§			return i
§§		}
§§	}
§§}
§§
§§func redoMedians(elms map[int]struct{}, medians []float64, splits [][2]int, data []Groupable) {
§§	for i := range elms {
§§		medians[i] = 0
§§		for _, elm := range data[splits[i][0]:splits[i][1]] {
§§			medians[i] += float64(elm.Order())
§§		}
§§		medians[i] = medians[i] / float64(splits[i][1]-splits[i][0])
§§	}
§§}
§§
§§// calculate the Davies–Bouldin index of our separation
§§// https://en.wikipedia.org/wiki/Davies%E2%80%93Bouldin_index
§§func ScoreJenksGroup(data []Groupable, split [][2]int) float64 {
§§	// calculate medians of all clusters
§§	var medians []float64 = make([]float64, len(split))
§§	for i := 0; i < len(split); i++ { // O(n), trotz doppelschleife
§§		for _, elm := range data[split[i][0]:split[i][1]] {
§§			medians[i] += float64(elm.Order())
§§		}
§§		medians[i] = medians[i] / float64(split[i][1]-split[i][0])
§§	}
§§
§§	// calculate scatter within the clusters: Si
§§	var Si []float64 = make([]float64, len(split))
§§	for i := 0; i < len(split); i++ {
§§		for _, elm := range data[split[i][0]:split[i][1]] {
§§			Si[i] += math.Abs(float64(elm.Order()) - medians[i])
§§		}
§§		Si[i] = Si[i] / float64(split[i][1]-split[i][0])
§§	}
§§
§§	// find Di
§§	var Di []float64 = make([]float64, len(split))
§§	ichan := make(chan int)
§§	dchan := make(chan struct{}, len(split))
§§	for routines := 0; routines < runtime.NumCPU()/2; routines++ {
§§		go func() {
§§			for {
§§				select {
§§				case i, more := <-ichan:
§§					if !more {
§§						return
§§					}
§§					// do stuff
§§					for j := 0; j < len(split); j++ {
§§						if i != j {
§§							rij := (Si[i] + Si[j]) / math.Abs(medians[i]-medians[j]) // Mij
§§							if rij > Di[i] {
§§								Di[i] = rij
§§							}
§§						}
§§					}
§§					// done stuff
§§					dchan <- struct{}{}
§§				}
§§			}
§§		}()
§§	}
§§
§§	for cnt := 0; cnt < len(split); cnt++ {
§§		var i int = cnt
§§		ichan <- i
§§	} // outer i/cnt loop
§§
§§	close(ichan)
§§	for cnt := 0; cnt < len(split); cnt++ {
§§		<-dchan
§§	} // outer i/cnt loop
§§
§§	// calculate DB:
§§	var DB float64
§§	for i := 0; i < len(split); i++ {
§§		DB += Di[i]
§§	}
§§	DB /= float64(len(split))
§§
§§	//fmt.Println("finished scoring jenks group")
§§	return DB
§§}
