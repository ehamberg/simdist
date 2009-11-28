#!/usr/bin/python -u
#
# Copyright 2009 Boye A. Hoeverstad
#
# This file is part of Simdist.
#
# Simdist is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Simdist is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Simdist.  If not, see <http://www.gnu.org/licenses/>.
#


"""Minimal Python master for OneMax estimation with Simdist.""" 

import sys, random, copy

class OneMaxIndiv():
    def __init__(self, length):
        self.genes = [random.randrange(0, 2) for i in range(length)]
        self.fitness = 0

    def Mutate(self, prob=0.025):
        for (idx, bit) in enumerate(self.genes):
            if random.uniform(0, 1) < prob:
                self.genes[idx] = abs(bit - 1)

def PrintGenome(indiv):
    for g in indiv.genes:
        print g,
    print

def BestIndiv(pop):
    return max(pop, key = lambda x: x.fitness)

def NextGeneration(pop):
    best = BestIndiv(pop)
    pop = [copy.deepcopy(best) for n in range(len(pop))]
    [indiv.Mutate() for indiv in pop]
    return pop

def EvaluatePopulation(pop):
    for indiv in pop:
        PrintGenome(indiv)
    for indiv in pop:
        indiv.fitness = int(sys.stdin.readline())

def PrintStatus(gen, pop):
    best = BestIndiv(pop)
    print >>sys.stderr, "Generation %d, max fitness %d: " %(gen, best.fitness),
    for g in best.genes:
        print >>sys.stderr, g,
    print >>sys.stderr, ""

def Evolution(pop, gens):
    for gen in range(gens):
        EvaluatePopulation(pop)
        PrintStatus(gen, pop)
        pop = NextGeneration(pop)
    PrintStatus(gen, pop)

def Main():
    pop = [OneMaxIndiv(100) for i in range(50)]
    Evolution(pop, 50)

if __name__ == "__main__":
    Main()
