#!/usr/bin/python

import os

import sys

import Queue


def makeWordDict( ):
    with open("wordsEn.txt") as word_file:
        wordDict = set(word.strip().lower() for word in word_file)

    return wordDict


wordDict = makeWordDict( )


class Node(object):
    def __init__(self, word):
        self.pload = word

        # Multiple parents possible
        self.parents = []

        self.children = []


    def dumpNode(self):
        print '\n'

        print 'pload: ', self.pload, 'parents: ',

        for item in self.parents:
            print item.pload,

        print ' children: ',

        for item in self.children:
            print item.pload,

        print '\n'


class wordTransitions(object):
    def __init__(self, startWord, endWord):
        self.startWord = startWord

        self.endWord = endWord

        self.startNode = None

        self.charlist = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

        self.qOfNodes = Queue.Queue()


    def _checkListForWord(self, word, inList):
        for item in inList:
            if word.lower() == item.lower():
                return True

        return False


    def _findNode(self, word, root):
        '''
        Find a node whose payload matches word (this is depth first)
        '''
        if root.pload == word:
            return root

        else:
            for chld in root.children:
                retval = self._findNode(word, chld)

                if retval != None:
                    return retval

        return None


    def _findChildren(self, word):
        '''
        wordDict is a set of legal dictionary words
        '''
        children = []

        word = word.lower()

        for rchar in self.charlist:
            for idx, char in enumerate(word):
                if char != rchar:
                    tstword = word[:idx] + rchar + word[idx+1:]

                    if tstword in wordDict:
                        children.append(tstword)

        return children


    def buildSubGraph(self, node):
        '''
        Build list of children.  For each, check if a node already
        exists, if not, create and append node to queue of nodes
        to check
        '''
        children = self._findChildren(node.pload)

        if self._checkListForWord(self.endWord, children) is True:
            # We are done - child of node has target word
            return True

        for child in children:
            # Lookiing for the word in whole graph - may not be optimal
            chnode = self._findNode(child, self.startNode)

            if chnode is not None:
                if chnode.pload != self.startWord:
                    # Add this node as parent, if not startWord
                    chnode.parents.append(node)

                # print chnode.dumpNode( )

            else:
                newnode = Node(child)

                newnode.parents.append(node)

                node.children.append(newnode)

                # newnode.dumpNode( )

                # add node to queue of nodes
                self.qOfNodes.put(newnode)

        return False


    def findPath(self):
        '''
        Build a graph with bredth first search
        '''
        self.startNode = Node(self.startWord)

        self.qOfNodes.put(self.startNode)

        node = None

        while self.qOfNodes.empty() is not True:
            node = self.qOfNodes.get()

            if self.buildSubGraph(node) is True:
                # We found the parent node of target word
                break

        if node == None:
            print 'Failed to find path from source word to dest word - bug!'

        else:
            # Walk up the graph
            print self.endWord

            while True:
                print node.pload

                # Arbitrarily choosing first parent
                if len(node.parents) > 0:
                    node = node.parents[0]

                else:
                    break


def main():

    wt = wordTransitions('cat', 'dog')

    wt.findPath( )


if __name__ == '__main__':
    main()

