import { describe, it, expect, vi } from 'vitest'
import { definition } from '../../src/components/blockly/mh_gamepad_buttonpressed.js'

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

describe('mh_gamepad_buttonpressed block definition', () => {
    it('has category "Gamepad"', () => {
        expect(definition.category).toBe('Gamepad')
    })

    it('has type "mh_gamepad_buttonpressed"', () => {
        expect(definition.blockdefinition.type).toBe('mh_gamepad_buttonpressed')
    })

    it('has output (expression block)', () => {
        expect('output' in definition.blockdefinition).toBe(true)
    })
})

describe('mh_gamepad_buttonpressed generator', () => {
    it('generates correct code for BUTTON_1 on GAMEPAD1', () => {
        const block = makeBlock({ BUTTON: 'GAMEPAD_BUTTON_1' })
        const gen = makeGenerator({ GAMEPAD: 'GAMEPAD1' })
        const result = definition.generator(block, gen)
        expect(result[0]).toBe('gamepad.buttonpressed(GAMEPAD1,GAMEPAD_BUTTON_1)')
        expect(result[1]).toBe(0)
    })

    it('generates correct code for BUTTON_5 on GAMEPAD1', () => {
        const block = makeBlock({ BUTTON: 'GAMEPAD_BUTTON_5' })
        const gen = makeGenerator({ GAMEPAD: 'GAMEPAD1' })
        const result = definition.generator(block, gen)
        expect(result[0]).toBe('gamepad.buttonpressed(GAMEPAD1,GAMEPAD_BUTTON_5)')
        expect(result[1]).toBe(0)
    })

    it('embeds gamepad and button in output', () => {
        const block = makeBlock({ BUTTON: 'GAMEPAD_BUTTON_10' })
        const gen = makeGenerator({ GAMEPAD: 'myGamepad' })
        const result = definition.generator(block, gen)
        expect(result[0]).toContain('myGamepad')
        expect(result[0]).toContain('GAMEPAD_BUTTON_10')
    })
})
