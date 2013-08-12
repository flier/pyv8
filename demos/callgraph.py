import os
import argparse
import json

import pydot

from PyV8 import *

__author__ = 'flier'


class CallGraph(object):
    def __init__(self, name):
        self.graph = pydot.Dot(name, graph_type='digraph')
        self.funcs = {}
        self.callstack = []

    @staticmethod
    def parseCmdLine():
        parser = argparse.ArgumentParser(description='Parse Javascript and Generate Call Graph')

        parser.add_argument("files", nargs=argparse.REMAINDER)

        return parser.parse_args()

    def parseScript(self, filename):
        print "Paring script <%s> ..." % filename

        with JSContext():
            JSEngine().compile(open(filename).read()).visit(self)

        return self

    def generateGraph(self):
        json.dump(self.json, open(self.graph.get_name() + ".json", "w+"), indent=4, separators=(',', ': '))

        self.graph.write(self.graph.get_name() + ".dot")
        self.graph.create(prog='dot', format='png')

        filename = self.graph.get_name() + ".png"

        if os.path.exists(filename):
            os.system("open " + filename)

    def onProgram(self, prog):
        self.json = json.loads(prog.toJSON())

        self.onFunctionLiteral(prog)

    def onBlock(self, block):
        for stmt in block.statements:
            stmt.visit(self)

    def onExpressionStatement(self, stmt):
        stmt.expression.visit(self)

    def onFunctionDeclaration(self, decl):
        self.onFunctionLiteral(decl.function)

    def onFunctionLiteral(self, func):
        name = func.name

        self.callstack.append(func)

        if len(name) == 0:
            name = 'anonymous$%d' % func.startPos

        node = self.funcs.get(name)

        if node is None:
            node = pydot.Node(name)

            self.graph.add_node(node)

            self.funcs[name] = node

        for decl in func.scope.declarations:
            decl.visit(self)

        for stmt in func.body:
            stmt.visit(self)

        self.callstack.pop()

    def onCall(self, expr):
        print expr.name


if __name__ == '__main__':
    opts = CallGraph.parseCmdLine()

    for file in opts.files:
        if not file.startswith('/'):
            file = os.path.normpath(os.path.join(os.getcwd(), file))

        name = os.path.splitext(os.path.basename(file))[0]

        CallGraph(name).parseScript(file).generateGraph()