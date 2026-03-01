import { describe, it, expect, vi } from 'vitest'
import { definition } from '../../src/components/blockly/lego_select_mode.js'

function makeBlock(fields = {}, values = {}, statements = {}, extra = {}) {
    return {
        getFieldValue: (name) => fields[name] ?? null,
        id: extra.id ?? 'test-block-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra
    }
}

function makeGenerator(values = {}, statements = {}) {
    return {
        valueToCode: (block, name, _order) => values[name] ?? '',
        statementToCode: (block, name) => statements[name] ?? ''
    }
}

describe('lego_select_mode block definition', () => {
    it('has category "LEGO©"', () => {
        expect(definition.category).toBe('LEGO©')
    })

    it('has type "lego_select_mode"', () => {
        expect(definition.blockdefinition.type).toBe('lego_select_mode')
    })

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true)
        expect(definition.blockdefinition.nextStatement).toBe(true)
    })

    it('has inputsForToolbox with shadow type "mh_port"', () => {
        expect(definition.inputsForToolbox).toBeDefined()
        expect(definition.inputsForToolbox.PORT).toBeDefined()
        expect(definition.inputsForToolbox.PORT.shadow.type).toBe('mh_port')
    })
})

describe('lego_select_mode generator', () => {
    it('generates "lego.selectmode(PORT1, 2)\\n" for PORT1 and mode 2', () => {
        const block = makeBlock()
        const gen = makeGenerator({ PORT: 'PORT1', MODE: '2' })
        const code = definition.generator(block, gen)
        expect(code).toBe('lego.selectmode(PORT1, 2)\n')
    })

    it('embeds port and mode in output', () => {
        const block = makeBlock()
        const gen = makeGenerator({ PORT: 'PORT3', MODE: '0' })
        const code = definition.generator(block, gen)
        expect(code).toContain('PORT3')
        expect(code).toContain('0')
    })

    it('ends with newline', () => {
        const block = makeBlock()
        const gen = makeGenerator({ PORT: 'PORT2', MODE: '1' })
        const code = definition.generator(block, gen)
        expect(code).toMatch(/\n$/)
    })
})
