# Diretrizes de Documentação Reutilizáveis

## Propósito
Este arquivo foi escrito para ser modular. Cada seção pode ser copiada para outro projeto e adaptada apenas pela troca dos nomes das camadas e da sintaxe de documentação.

## Regra Principal
Ao criar ou editar qualquer arquivo, siga a convenção de documentação dominante do diretório e da linguagem. Não invente um padrão novo e não reescreva comentários só para padronizar.

## Como Reutilizar
- Substitua os exemplos de caminhos pelas camadas equivalentes do projeto atual.
- Se o projeto não usar Doxygen, mantenha os mesmos princípios e adapte apenas a sintaxe.
- Trate "interface pública", "camada interna" e "implementação" como papéis; o diretório concreto pode mudar.
- Mantenha cada seção independente; se uma parte não se aplicar ao novo projeto, remova-a sem quebrar o restante.
- Priorize regras de contrato e arquitetura em vez de detalhes específicos do repositório atual.

## Mapeamento de Camadas
- Interface pública: o que consumidores externos usam e o que define contrato estável.
- Camada interna: suporte compartilhado, helpers e declarações que existem para sustentar a interface, não para expô-la.
- Implementação: código executável, exemplos, testes e utilitários locais.
- Se o projeto tiver outro layout, mapeie esses papéis para as pastas equivalentes antes de aplicar estas regras.

## Prioridade
1. Convenção do arquivo ou módulo mais próximo.
2. Convenção da pasta ou camada mais próxima.
3. Regra geral do workspace.
4. Em caso de dúvida, siga o padrão da interface pública do projeto.

## Escopo Prático
- Use o estilo dominante do projeto para documentação relevante ao contrato.
- Evite introduzir novos blocos de documentação em código interno, exemplos e testes se o projeto normalmente usa comentários inline nesses trechos.
- Para arquivos novos, copie a convenção predominante do local mais próximo em vez de inventar uma forma nova de documentar.
- Quando a base já documenta em português, mantenha a documentação em português.
- Não reescreva comentários apenas para padronizar; prefira mudanças que esclareçam contrato, uso ou restrições.
- Se o projeto não usar Doxygen, preserve os mesmos conceitos e troque apenas a marcação.

## Arquitetura
- Preserve a separação entre interface pública, camadas internas e implementação; a documentação deve reforçar essa fronteira, não borrá-la.
- Documente dependências e direção de responsabilidade quando isso impactar a leitura do código, especialmente entre camadas equivalentes do projeto.
- Em APIs públicas, destaque o contrato estável que consumidores externos precisam respeitar; em internals, destaque apenas o que explica a decisão de arquitetura.
- Quando um símbolo existir para servir outro módulo, deixe isso explícito na documentação para evitar duplicação de lógica ou uso fora de contexto.
- Prefira registrar invariantes, ownership, ciclo de vida e fluxo de dados em vez de descrições literais de funções.
- Se uma mudança introduzir acoplamento novo, documente a razão e a limitação para que agentes futuros não normalizem uma solução frágil.
- Mantenha a documentação coerente com a estrutura do projeto; evite textos que incentivem atalhos ou uso cruzado indevido entre camadas.

## O Que Documentar
- APIs públicas, tipos exportados, structs, enums, macros e declarações que formam a interface do projeto.
- Para cada `struct`, coloque o bloco de documentação antes da definição, liste os campos com `@param` quando isso ajudar a explicar o contrato e prefira um exemplo como `Mat4`: comentário acima da definição, sem comentário residual no fim da linha `} Tipo;`.
- Campos de `struct` quando eles carregam estado, ownership, ciclo de vida, flags, handles, buffers, dimensões, índices, contagens ou qualquer outro contrato que não fique óbvio só pelo nome do campo.
- Regras de posse, vida útil, mutabilidade, limites, unidade de medida, formato de dados, pré-condições e efeitos colaterais.
- Casos de erro, retorno nulo, pós-condições, invariantes e observações de thread-safety quando isso impactar o chamador.
- Campos de estrutura com anotação inline quando isso já for prática do arquivo.
- Exemplos curtos quando eles realmente ajudam a entender o contrato.
- Layout de dados quando a ordem ou o formato importarem: arrays contíguos, stride, ordem de matrizes, componentes de vetores, índices triangulados, caches e tabelas de dispatch.

## Tags de Documentação
- `@brief`: resumo curto e direto da intenção do símbolo.
- `@details`: contexto adicional quando o resumo não basta.
- `@param`: descreve parâmetros, incluindo papel, unidade, intervalo e ownership quando relevante.
- `@return` e `@retval`: valor de retorno, especialmente quando houver erro, status ou ponteiros nulos.
- `@note`: restrições, armadilhas, efeitos colaterais, limitações e expectativas do chamador.
- `@warning`: riscos de uso incorreto ou estados perigosos.
- `@pre` e `@post`: pré e pós-condições que o chamador precisa respeitar.
- `@see` e `@sa`: símbolos relacionados, fluxo alternativo ou ponto de extensão.
- `@file`: cabeçalho de arquivo quando o padrão local usa isso.
- `@author` e `@date`: apenas quando o arquivo já adota esse metadado.
- `@ingroup` e `@defgroup`: agrupamento quando houver navegação por módulos.
- `@code` e `@endcode`: exemplos curtos e realmente úteis.
- `@deprecated`: APIs legadas.
- `@todo`: somente quando o trabalho estiver propositalmente incompleto.
- `@tparam`: templates ou genéricos, se aparecerem.

## Como Agir Por Camada
### Interface pública
- Mantenha documentação na sintaxe dominante do projeto para funções, tipos exportados e declarações públicas.
- Preserve `@file`, `@author` e `@date` quando o arquivo já seguir esse padrão.
- Documente o contrato do símbolo: intenção, posse, vida útil, limites, formato, unidades, erros e efeitos colaterais.
- Para `struct` e `enum`, documente cada campo ou valor quando o nome sozinho não deixar claro o papel ou o impacto no contrato.
- Para agregados como buffers, matrizes, caches e tabelas de funções, explique o layout e o significado dos componentes no próprio ponto de declaração.
- Use `@brief` curto e, se necessário, complemente com `@details`.
- Use `@param`, `@return` e `@retval` quando o retorno exigir mais precisão.
- Use `@note`, `@warning`, `@pre`, `@post` e `@see` quando houver restrições ou relações importantes.
- Para campos de estrutura, use anotação inline quando essa for a convenção do arquivo.

### Camada interna
- Siga o mesmo idioma e a mesma estrutura da interface pública quando isso fizer sentido, mas com menos metadados e menos verbosidade.
- Documente funções e tipos internos somente quando a intenção, as restrições ou o contrato não estiverem óbvios.
- Use as tags relevantes quando elas agregarem clareza real.
- Preserve cabeçalhos de arquivo apenas quando o arquivo já adota esse padrão.
- Prefira explicar comportamento, invariantes e dependências em vez de repetir o nome do símbolo.

### Implementação, exemplos e testes
- Evite adicionar blocos de documentação em funções locais, helpers privados e trechos triviais.
- Preserve blocos existentes apenas quando eles descrevem um contrato que o chamador realmente precisa conhecer.
- Use comentários inline curtos para explicar passos de algoritmo, dependências, formatos de dados, estados transitórios ou decisões não óbvias.
- Documente o "por quê" e o contrato de uso, não uma descrição literal do código.
- Em `struct` internas ou de estado local, comente os campos que representam ownership, handles, caches, flags, dimensões, stride, contadores, offsets, índices ou valores derivados; deixe sem comentário apenas o que seja realmente autoexplicativo.
- Se uma função pública for implementada aqui e ainda não tiver documentação na declaração, documente a interface no ponto mais apropriado do projeto.
- Não faça limpeza cosmética de comentários existentes sem necessidade funcional.

## Objetivo
Reduzir trabalho manual de documentação e manter consistência com a documentação majoritária do repositório, sem engessar a evolução do código. A documentação também deve ajudar a preservar uma arquitetura limpa e previsível.
